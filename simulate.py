import numpy as np
import soundfile as sf
import math

class LPC:
    def __init__(self, num_channels, order=32, frame_len=512, hop_size=256, buf_len=1024, exlen=4096):
        self.ORDER = order
        self.FRAMELEN = frame_len
        self.HOPSIZE = hop_size
        self.BUFLEN = buf_len
        self.EXLEN = exlen

        self.inPtrs = [0] * num_channels
        self.smpCnts = [0] * num_channels
        self.outWtPtrs = [0] * num_channels
        self.outRdPtrs = [0] * num_channels
        self.exPtrs = [0] * num_channels
        self.histPtrs = [0] * num_channels
        self.exCntPtrs = [0] * num_channels

        self.max_amp = 1
        self.phi = [0.0] * (self.ORDER + 1)
        self.alphas = [0.0] * (self.ORDER + 1)
        self.prev_alphas = [0.0] * (self.ORDER + 1)
        self.reflections = [0.0] * self.ORDER
        self.orderedInBuf = [0.0] * self.FRAMELEN
        self.window = [0.5 * (1.0 - math.cos(2.0 * math.pi * i / (self.FRAMELEN - 1))) for i in range(self.FRAMELEN)]

        self.inBuf = [[0.0] * self.BUFLEN for _ in range(num_channels)]
        self.outBuf = [[0.0] * self.BUFLEN for _ in range(num_channels)]
        self.out_hist = [[0.0] * self.ORDER for _ in range(num_channels)]

        self.reset_a()

    def reset_a(self):
        self.alphas[0] = 1.0
        self.prev_alphas[0] = 1.0
        for i in range(1, self.ORDER + 1):
            self.alphas[i] = 0.0
            self.prev_alphas[i] = 0.0

    def autocorrelate(self, x, lag):
        return sum(x[n] * x[n + lag] for n in range(len(x) - lag))

    def levinson_durbin(self):
        self.reset_a()
        E = self.phi[0]
        for k in range(self.ORDER):
            lbda = -sum(self.alphas[j] * self.phi[k + 1 - j] for j in range(k + 1))
            lbda /= E
            self.reflections[k] = lbda / 10.0
            for n in range((k + 1) // 2 + 1):
                tmp = self.alphas[k + 1 - n] + lbda * self.alphas[n]
                self.alphas[n] += lbda * self.alphas[k + 1 - n]
                self.alphas[k + 1 - n] = tmp
            E *= (1 - lbda * lbda)

        max_k = max(abs(k) for k in self.reflections)
        self.reflections = [k / max_k for k in self.reflections]

        for i in range(self.ORDER):
            self.alphas[i] = self.reflections[i]
            if i > 0:
                for j in range(1, i):
                    self.alphas[j] -= self.reflections[i] * self.prev_alphas[i - j]
            self.prev_alphas[:self.ORDER] = self.alphas[:self.ORDER]

    def applyLPC(self, inout, lpcMix, exPercentage, ch, exStartPos):
        inPtr = self.inPtrs[ch]
        smpCnt = self.smpCnts[ch]
        outWtPtr = self.outWtPtrs[ch]
        outRdPtr = self.outRdPtrs[ch]
        exPtr = self.exPtrs[ch]
        exCntPtr = self.exCntPtrs[ch]
        histPtr = self.histPtrs[ch]
        exStart = int(exStartPos * self.EXLEN)

        for s in range(len(inout)):
            self.inBuf[ch][inPtr] = inout[s]
            inPtr = (inPtr + 1) % self.BUFLEN

            out = self.outBuf[ch][outRdPtr]
            inout[s] = lpcMix * out + (1 - lpcMix) * inout[s]
            self.outBuf[ch][outRdPtr] = 0.0
            outRdPtr = (outRdPtr + 1) % self.BUFLEN

            smpCnt += 1
            if smpCnt >= self.HOPSIZE:
                smpCnt = 0
                for i in range(self.FRAMELEN):
                    inBufIdx = (inPtr + i - self.FRAMELEN + self.BUFLEN) % self.BUFLEN
                    self.orderedInBuf[i] = self.window[i] * self.inBuf[ch][inBufIdx]
                for lag in range(self.ORDER + 1):
                    self.phi[lag] = self.autocorrelate(self.orderedInBuf, lag)

                if self.phi[0] != 0:
                    self.levinson_durbin()
                    G = self.phi[0]
                    for k in range(self.ORDER):
                        G -= self.alphas[k + 1] * self.phi[k + 1]
                    G = math.sqrt(G)

                    for n in range(self.FRAMELEN):
                        ex = self.noise[exPtr]
                        exPtr = (exPtr + 1) % self.EXLEN
                        exCntPtr += 1
                        if exCntPtr >= int(exPercentage * self.EXLEN):
                            exCntPtr = 0
                            exPtr = exStart

                        out_n = G * ex
                        for k in range(self.ORDER):
                            idx = (histPtr + k) % self.ORDER
                            out_n += self.alphas[k + 1] * self.out_hist[ch][idx]
                        histPtr = (histPtr - 1) % self.ORDER
                        self.out_hist[ch][histPtr] = out_n

                        wtIdx = (outWtPtr + n) % self.BUFLEN
                        self.outBuf[ch][wtIdx] += out_n

                    self.out_hist[ch] = [0.0] * self.ORDER
                    histPtr = 0
                    outWtPtr = (outWtPtr + self.HOPSIZE) % self.BUFLEN

        self.inPtrs[ch] = inPtr
        self.smpCnts[ch] = smpCnt
        self.outWtPtrs[ch] = outWtPtr
        self.outRdPtrs[ch] = outRdPtr
        self.exPtrs[ch] = exPtr
        self.exCntPtrs[ch] = exCntPtr
        self.histPtrs[ch] = histPtr
