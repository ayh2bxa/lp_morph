import math
import numpy as np
import soundfile as sf

# -----------------------------
# LPC class (translated from C++)
# -----------------------------
class LPC:
    def __init__(self, numChannels, noise,
                 ORDER=10, FRAMELEN=1024, BUFLEN=2048,
                 HOPSIZE=256, EXLEN=4096,
                 exTypeChanged=False, orderChanged=False):
        self.numChannels = numChannels
        self.noise = noise  # expected to be a 1D list or np.array of floats (mono)
        self.ORDER = ORDER
        self.FRAMELEN = FRAMELEN
        self.BUFLEN = BUFLEN
        self.HOPSIZE = HOPSIZE
        self.EXLEN = EXLEN
        self.exTypeChanged = exTypeChanged
        self.orderChanged = orderChanged

        # Pointer vectors for each channel
        self.inPtrs = [0] * numChannels
        self.smpCnts = [0] * numChannels
        self.outWtPtrs = [HOPSIZE] * numChannels
        self.outRdPtrs = [0] * numChannels
        self.exPtrs = [0] * numChannels
        self.histPtrs = [0] * numChannels
        self.exCntPtrs = [0] * numChannels

        self.max_amp = 1

        # LPC processing arrays
        self.phi = np.array([0.0] * (ORDER + 1))
        self.phi = np.array([
        0.0011650967644527555,
        0.0011243076578477997,
        0.0010183736016510155,
        0.000980842337886674,
        0.0009079763990920253,
        0.0008495362665627827,
        0.0008949163939777697,
        0.0007906173902333849,
        0.0007305769888115067,
        0.0006528870110322423,
        0.0005926066666382289,
        0.0005230877063955064,
        0.0004712608939163837,
        0.0004317071728429491,
        0.0003753278812374771,
        0.0003267771126703321,
        0.0002971448763450384,
        0.0002391124700087236,
        0.0001886364288442284,
        0.0001490962861505826,
        0.00010788938742510259,
        8.137553121819049e-05,
        6.321871960133731e-05,
        4.658054978374911e-05,
        3.474539502960279e-05,
        2.3869690046633524e-05,
        1.7648430336684734e-05,
        1.2455921366106452e-05,
        9.162902167414978e-06,
        6.916217623507664e-06,
        5.225410923638218e-06,
        3.750263107387992e-06,
        2.5076934922300722e-06
])
        self.alphas = np.array([0.0] * (ORDER + 1))
        self.prev_alphas = [0.0] * (self.ORDER + 1)
        self.orderedInBuf = [0.0] * FRAMELEN
        self.window = [0.0] * FRAMELEN
        self.reflections = [0.0] * self.ORDER

        # 2D buffers: one per channel
        self.inBuf = [[0.0] * BUFLEN for _ in range(numChannels)]
        self.outBuf = [[0.0] * BUFLEN for _ in range(numChannels)]
        self.out_hist = [[0.0] * ORDER for _ in range(numChannels)]

        # Initialize the window with a Hann window
        for i in range(FRAMELEN):
            self.window[i] = 0.5 * (1.0 - math.cos(2.0 * math.pi * i / (FRAMELEN - 1)))

        self.reset_a()

    def reset_a(self):
        self.alphas[0] = 1.0
        self.prev_alphas[0] = 1.0
        for i in range(1, self.ORDER + 1):
            self.alphas[i] = 0.0
            self.prev_alphas[i] = 0.0

    def levinson_durbin(self):
        self.reset_a()
        E = self.phi[0]
        for k in range(self.ORDER):
            lbda = -sum(self.alphas[j] * self.phi[k + 1 - j] for j in range(k + 1))
            lbda /= E
            self.reflections[k] = lbda
            for n in range((k + 1) // 2 + 1):
                tmp = self.alphas[k + 1 - n] + lbda * self.alphas[n]
                self.alphas[n] += lbda * self.alphas[k + 1 - n]
                self.alphas[k + 1 - n] = tmp
            E *= (1 - lbda * lbda)
        # print(self.alphas)
        # print(self.reflections)
        # exit()

    def autocorrelate(self, x, lag):
        res = 0.0
        for n in range(len(x) - lag):
            res += x[n] * x[n + lag]
        return res

    def applyLPC(self, inout, numSamples, lpcMix, exPercentage, ch, exStartPos):
        """
        inout:      Mutable sequence (list) of floats containing audio samples for channel ch.
        numSamples: Number of samples to process in inout.
        lpcMix:     Mix factor for LPC output vs. direct input.
        exPercentage: Fraction of the excitation used per hop (range 0..1).
        ch:         Current channel index.
        exStartPos: Start position in the excitation buffer (0..1).
        """
        self.levinson_durbin()
        # exit()
        inPtr = self.inPtrs[ch]
        smpCnt = self.smpCnts[ch]
        outWtPtr = self.outWtPtrs[ch]
        outRdPtr = self.outRdPtrs[ch]

        if self.exTypeChanged:
            self.exPtrs[ch] = int(self.EXLEN * exStartPos)
            self.exCntPtrs[ch] = 0
            self.histPtrs[ch] = 0

        if self.orderChanged:
            for i in range(len(self.out_hist[ch])):
                self.out_hist[ch][i] = 0.0

        exPtr = self.exPtrs[ch]
        exCntPtr = self.exCntPtrs[ch]
        histPtr = self.histPtrs[ch]
        exStart = int(exStartPos * self.EXLEN)
        H = 0
        for s in range(numSamples):
            # Write input sample into the input buffer.
            self.inBuf[ch][inPtr] = inout[s]
            inPtr += 1
            if inPtr >= self.BUFLEN:
                inPtr = 0

            # Read from the output buffer and mix with the input sample.
            out_val = self.outBuf[ch][outRdPtr]
            inout[s] = lpcMix * out_val + (1 - lpcMix) * inout[s]
            self.outBuf[ch][outRdPtr] = 0.0
            outRdPtr += 1
            if outRdPtr >= self.BUFLEN:
                outRdPtr = 0
            smpCnt += 1
            if smpCnt >= self.HOPSIZE:
                # self.ORDER = 1+(self.ORDER+1)%100
                smpCnt = 0
                # Build the ordered input buffer using the window.
                for i in range(self.FRAMELEN):
                    inBufIdx = (inPtr + i - self.FRAMELEN + self.BUFLEN) % self.BUFLEN
                    self.orderedInBuf[i] = self.window[i] * self.inBuf[ch][inBufIdx]
                # Compute autocorrelation.
                # for lag in range(self.ORDER + 1):
                #     self.phi[lag] = self.autocorrelate(self.orderedInBuf, lag)
                if self.phi[0] != 0:
                    self.levinson_durbin()
                    if H < 10:
                        # print(self.alphas)
                        H += 1
                    G = self.phi[0]
                    for k in range(self.ORDER):
                        G -= self.alphas[k + 1] * self.phi[k + 1]
                    G = math.sqrt(G)
                    print(G)
                    # if H < 10:
                    #     print(self.alphas)
                    # err = 0
                    # for n in range(self.FRAMELEN):
                    #     residual = self.orderedInBuf[n]
                    #     x_hat_n = 0
                    #     for k in range(1, self.ORDER+1):
                    #         if n-k >= 0:
                    #             x_hat_n += self.alphas[k]*self.orderedInBuf[n-k]
                    #     residual -= x_hat_n
                    #     residual *= residual
                    #     err += residual
                    # G = math.sqrt(err)
                    # print(G)
                    # G = sum(self.alphas)
                    for n in range(self.FRAMELEN):
                        ex = G*self.noise[exPtr]
                        exPtr = (exPtr + 1) % self.EXLEN
                        exCntPtr += 1
                        if exCntPtr >= int(exPercentage * self.EXLEN):
                            exCntPtr = 0
                            exPtr = exStart
                        out_n = ex
                        for k in range(self.ORDER):
                            idx = (histPtr + k) % self.ORDER
                            out_n -= self.alphas[k + 1] * self.out_hist[ch][idx]
                        histPtr = (histPtr - 1) % self.ORDER
                        self.out_hist[ch][histPtr] = out_n

                        wtIdx = (outWtPtr + n) % self.BUFLEN
                        self.outBuf[ch][wtIdx] += out_n
                    # print(self.outBuf[ch])
                    for i, val in enumerate(self.outBuf[ch]):
                        print(i, val)
                    exit()
                    self.out_hist[ch] = [0.0] * self.ORDER
                    histPtr = 0
                    outWtPtr = (outWtPtr + self.HOPSIZE) % self.BUFLEN

        # Store updated pointers.
        self.inPtrs[ch] = inPtr
        self.smpCnts[ch] = smpCnt
        self.outWtPtrs[ch] = outWtPtr
        self.outRdPtrs[ch] = outRdPtr
        self.exPtrs[ch] = exPtr
        self.histPtrs[ch] = histPtr


# -----------------------------
# High-Pass Filter (DC Blocker)
# -----------------------------
def highpass_filter(signal, R=0.995):
    """
    Apply a one-pole high-pass filter to remove DC offset.
    y[n] = x[n] - x[n-1] + R * y[n-1]
    """
    filtered = np.empty_like(signal)
    y_prev = 0.0
    x_prev = 0.0
    for n, x in enumerate(signal):
        y = x - x_prev + R * y_prev
        filtered[n] = y
        x_prev = x
        y_prev = y
    return filtered


# -----------------------------
# Process file function (simulating processBlock),
# using soundfile for I/O, and a stereo excitation
# file that is converted to mono.
# -----------------------------
def process_audio_file(
    input_file,         # Path to input audio file
    excitation_file,    # Path to stereo excitation file
    output_file         # Path to output audio file
):
    # --- Read main input using soundfile ---
    data, samplerate = sf.read(input_file, dtype='float32')
    # Ensure data is 2D (samples x channels)
    if data.ndim == 1:
        data = np.stack([data, data], axis=-1)
    numSamples, numChannels = data.shape

    # --- Read the stereo excitation and convert to mono ---
    exc_data, exc_samplerate = sf.read(excitation_file, dtype='float32')
    if exc_data.ndim == 1:
        mono_exc = exc_data
    else:
        mono_exc = np.mean(exc_data, axis=-1)
    
    EXLEN = len(exc_data)

    # ---------------
    # Set LPC parameters
    # ---------------
    currentGain = 1.0       # Pre-gain
    lpcMix = 1.0            # LPC mix factor (1.0 = fully processed)
    percentage = 1.0        # Fraction of excitation used per hop
    exStartPos = 0.0        # Start position in the excitation
    ORDER = 32             # LPC order
    FRAMELEN = 442          # Frame length
    BUFLEN = 2 * FRAMELEN   # Buffer length
    HOPSIZE = FRAMELEN // 2 # Overlap

    # Create LPC instance.
    lpc = LPC(
        numChannels=numChannels,
        noise=mono_exc,
        ORDER=ORDER,
        FRAMELEN=FRAMELEN,
        BUFLEN=BUFLEN,
        HOPSIZE=HOPSIZE,
        EXLEN=EXLEN,
        exTypeChanged=False,
        orderChanged=False
    )

    # Prepare output array.
    output = np.copy(data)

    # Process each channel.
    for ch in range(numChannels):
        output[:, ch] *= currentGain
        channel_data = output[:, ch].tolist()
        lpc.applyLPC(
            inout=channel_data,
            numSamples=len(channel_data),
            lpcMix=lpcMix,
            exPercentage=percentage,
            ch=ch,
            exStartPos=exStartPos
        )
        for idx, value in enumerate(channel_data):
            if value > 0 and math.isinf(value):
                print("pos inf at Index:", idx, "Entry:", value)
            if value < 0 and math.isinf(value):
                print("neg inf at Index:", idx, "Entry:", value)
        output[:, ch] = channel_data

    # # --- Apply high-pass filter to each channel to remove DC offset ---
    # for ch in range(numChannels):
    #     output[:, ch] = highpass_filter(output[:, ch], R=0.995)

    # --- Write the output file as 32-bit float WAV ---
    output /= (1.05*np.max(np.abs(output)))
    # output /= 100.0
    # output = np.clip(output, -0.95, 0.95)
    sf.write(output_file, output, samplerate, subtype='FLOAT')
    print(f"Processed audio written to: {output_file}")


# -----------------------------
# Example usage
# -----------------------------
if __name__ == "__main__":
    input_wav = "input2.wav"            # Your main input file
    excitation_wav = "resources/excitations/WhiteNoise.wav"    # Your stereo excitation file
    output_wav = "output2.wav"          # Output path
    process_audio_file(input_wav, excitation_wav, output_wav)
