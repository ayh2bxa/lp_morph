#include "lpc.h"

LPC::LPC(int numChannels) {
    inPtrs.resize(numChannels);
    smpCnts.resize(numChannels);
    outWtPtrs.resize(numChannels);
    outRdPtrs.resize(numChannels);
    exPtrs.resize(numChannels);
    histPtrs.resize(numChannels);
    exCntPtrs.resize(numChannels);
    for (int ch = 0; ch < numChannels; ch++) {
        inPtrs[ch] = 0;
        smpCnts[ch] = 0;
        outWtPtrs[ch] = HOPSIZE;
        outRdPtrs[ch] = 0;
        exPtrs[ch] = 0;
        histPtrs[ch] = 0;
        exCntPtrs[ch] = 0;
    }
    max_amp = 1;
    phi.resize(ORDER+1);
    alphas.resize(ORDER+1);
    reflections.resize(ORDER);
    orderedInBuf.resize(FRAMELEN);
    window.resize(FRAMELEN);
    inBuf.resize(numChannels);
    outBuf.resize(numChannels);
    out_hist.resize(numChannels);
    for (int ch = 0; ch < numChannels; ch++) {
        inBuf[ch].resize(BUFLEN);
        outBuf[ch].resize(BUFLEN);
        out_hist[ch].resize(ORDER);
        for (int i = 0; i < BUFLEN; i++) {
            inBuf[ch][i] = 0.0;
            outBuf[ch][i] = 0.0;
        }
        for (int i = 0; i < ORDER; i++) {
            out_hist[ch][i] = 0.0;
        }
    }
    for (int i = 0; i < FRAMELEN; i++) {
        window[i] = 0.5*(1.0-cos(2.0*M_PI*i/(double)(FRAMELEN-1)));
    }
    for (int i = 0; i < FRAMELEN; i++) {
        orderedInBuf[i] = 0.0;
    }
    for (int i = 0; i < ORDER; i++) {
        phi[i] = 0.0;
    }
    phi[ORDER] = 0.0;
    reset_a();
}

//http://www.emptyloop.com/technotes/A%20tutorial%20on%20linear%20prediction%20and%20Levinson-Durbin.pdf
void LPC::levinson_durbin() {
    reset_a();
    double E = phi[0];
    for (int k = 0; k < ORDER; k++) {
        double lbda = 0.0;
        for (int j = 0; j <= k; j++) {
            lbda += alphas[j] * phi[k + 1 - j];
        }
        lbda = -lbda / E;
        reflections[k] = lbda;
        int half = (k + 1) / 2;
        for (int n = 0; n <= half; n++) {
            double tmp = alphas[k + 1 - n] + lbda * alphas[n];
            alphas[n] += lbda * alphas[k + 1 - n];
            alphas[k + 1 - n] = tmp;
        }
        E *= (1.0 - lbda * lbda);
    }
}

double LPC::autocorrelate(const vector<double>& x, int lag) {
    double res = 0.0;
    for (int n = 0; n < x.size()-lag; n++) {
        res += x[n]*x[n+lag];
    }
    return res;
}

void LPC::applyLPC(float *inout, int numSamples, float lpcMix, float exPercentage, int ch, float exStartPos) {
    inPtr = inPtrs[ch];
    smpCnt = smpCnts[ch];
    outWtPtr = outWtPtrs[ch];
    outRdPtr = outRdPtrs[ch];
    int exStart = static_cast<int>(exStartPos*EXLEN);
    if (exTypeChanged) {
        exPtrs[ch] = exStart;
        exCntPtrs[ch] = 0;
        histPtrs[ch] = 0;
    }
    if (orderChanged) {
        for (int i = 0; i < out_hist[ch].size(); i++) {
            out_hist[ch][i] = 0;
        }
        histPtrs[ch] = 0;
    }
    exPtr = exPtrs[ch];
    exCntPtr = exCntPtrs[ch];
    histPtr = histPtrs[ch];
    double factor = 1.0;
    for (int s = 0; s < numSamples; s++) {
        inBuf[ch][inPtr] = inout[s];
        inPtr++;
        if (inPtr >= BUFLEN) {
            inPtr = 0;
        }
        float out = outBuf[ch][outRdPtr];
        float in = inout[s];
        inout[s] = lpcMix*factor*out+(1-lpcMix)*in;
        outBuf[ch][outRdPtr] = 0;
        outRdPtr++;
        if (outRdPtr >= BUFLEN) {
            outRdPtr = 0;
        }
        smpCnt++;
        if (smpCnt >= HOPSIZE) {
            smpCnt = 0;
            double rms_in = 0.0;
            for (int i = 0; i < FRAMELEN; i++) {
                int inBufIdx = (inPtr+i-FRAMELEN+BUFLEN)%BUFLEN;
                orderedInBuf[i] = window[i]*inBuf[ch][inBufIdx];
            }
            if (matchInLevel) {
                for (int i = 0; i < FRAMELEN; i++) {
                    double smp = orderedInBuf[i];
                    rms_in += smp*smp;
                }
                rms_in = sqrt(rms_in/FRAMELEN);
            }
            for (int lag = 0; lag < ORDER+1; lag++) {
                phi[lag] = autocorrelate(orderedInBuf, lag);
            }
            if (phi[0] != 0) {
                levinson_durbin();
                double G;
                if (!matchInLevel) {
                    G = phi[0];
                    for (int k = 0; k < ORDER; k++) {
                        G -= alphas[k+1]*phi[k+1];
                    }
                    G = sqrt(G);
                }
                else {
                    G = 1.0;
                }
                double rms_out = 0.0;
                for (int n = 0; n < FRAMELEN; n++) {
                    double ex = (*noise)[exPtr];
                    exCntPtr++;
                    exPtr++;
                    if (exCntPtr >= static_cast<int>(exPercentage*EXLEN)) {
                        exCntPtr = 0;
                        exPtr = exStart;
                    }
                    if (exPtr >= EXLEN) {
                        exPtr = 0;
                    }
                    double out_n = G*ex;
                    for (int k = 0; k < ORDER; k++) {
                        int idx = (histPtr+k)%ORDER;
                        out_n -= alphas[k+1]*out_hist[ch][idx];
                    }
                    histPtr--;
                    if (histPtr < 0) {
                        histPtr += ORDER;
                    }
                    out_hist[ch][histPtr] = out_n;
                    unsigned long wtIdx = (outWtPtr+n)%BUFLEN;
                    outBuf[ch][wtIdx] += out_n;
                }
                if (matchInLevel) {
                    for (int n = 0; n < FRAMELEN; n++) {
                        unsigned long wtIdx = (outWtPtr+n)%BUFLEN;
                        rms_out += outBuf[ch][wtIdx]*outBuf[ch][wtIdx];
                    }
                    rms_out = sqrt(rms_out/FRAMELEN);
                    if (rms_in == 0) {
                        factor = 1.0;
                    }
                    else {
                        factor = rms_in/rms_out;
                    }
                }
                else {
                    factor = 1.0;
                }
                for (int i = 0; i < out_hist[ch].size(); i++) {
                    out_hist[ch][i] = 0;
                }
                histPtrs[ch] = 0;
                outWtPtr += HOPSIZE;
                if (outWtPtr >= BUFLEN) {
                    outWtPtr -= BUFLEN;
                }
            }
        }
    }
    inPtrs[ch] = inPtr;
    smpCnts[ch] = smpCnt;
    outWtPtrs[ch] = outWtPtr;
    outRdPtrs[ch] = outRdPtr;
    exPtrs[ch] = exPtr;
    histPtrs[ch] = histPtr;
}

void LPC::reset_a() {
    alphas[0] = 1.0;
    for (int i = 1; i < ORDER+1; i++) {
        alphas[i] = 0.0;
    }
}
