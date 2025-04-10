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
                smpCnt = 0
                # Build the ordered input buffer using the window.
                for i in range(self.FRAMELEN):
                    inBufIdx = (inPtr + i - self.FRAMELEN + self.BUFLEN) % self.BUFLEN
                    self.orderedInBuf[i] = self.window[i] * self.inBuf[ch][inBufIdx]
                # Compute autocorrelation.
                for lag in range(self.ORDER + 1):
                    self.phi[lag] = self.autocorrelate(self.orderedInBuf, lag)
                if self.phi[0] != 0.0:
                    self.levinson_durbin()
                    G = self.phi[0]
                    for k in range(self.ORDER):
                        G -= self.alphas[k + 1] * self.phi[k + 1]
                    G = math.sqrt(G)
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
    percentage = 0.0054        # Fraction of excitation used per hop
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
