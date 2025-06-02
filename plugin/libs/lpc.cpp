#include "lpc.h"

LPC::LPC(int numChannels) {
    totalNumChannels = numChannels;
    inWtPtrs.resize(numChannels);
    inRdPtrs.resize(numChannels);
    smpCnts.resize(numChannels);
    outWtPtrs.resize(numChannels);
    outRdPtrs.resize(numChannels);
    exPtrs.resize(numChannels);
    histPtrs.resize(numChannels);
    exCntPtrs.resize(numChannels);
    for (int ch = 0; ch < numChannels; ch++) {
        inWtPtrs[ch] = 0;
        smpCnts[ch] = 0;
        outWtPtrs[ch] = HOPSIZE;
        outRdPtrs[ch] = 0;
        exPtrs[ch] = 0;
        histPtrs[ch] = 0;
        exCntPtrs[ch] = 0;
        inRdPtrs[ch] = 0;
    }
    phi.resize(ORDER+1);
    alphas.resize(ORDER+1);
    reflections.resize(ORDER);
    orderedInBuf.resize(SAMPLERATE*0.01);
    orderedScBuf.resize(SAMPLERATE*0.01);
    window.resize(SAMPLERATE*0.01);
    inBuf.resize(numChannels);
    outBuf.resize(numChannels);
    out_hist.resize(numChannels);
    scBuf.resize(numChannels);
    for (int ch = 0; ch < numChannels; ch++) {
        inBuf[ch].resize(BUFLEN);
        scBuf[ch].resize(BUFLEN);
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
    for (int i = 0; i < orderedInBuf.size(); i++) {
        orderedInBuf[i] = 0.0;
        orderedScBuf[i] = 0.0;
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

double LPC::autocorrelate(const vector<double>& x, int frameSize, int lag) {
    double res = 0.0;
    for (int n = 0; n < frameSize-lag; n++) {
        res += x[n]*x[n+lag];
    }
    return res;
}

void LPC::prepareToPlay() {
    inWtPtrs.resize(totalNumChannels);
    inRdPtrs.resize(totalNumChannels);
    smpCnts.resize(totalNumChannels);
    outWtPtrs.resize(totalNumChannels);
    outRdPtrs.resize(totalNumChannels);
    exPtrs.resize(totalNumChannels);
    histPtrs.resize(totalNumChannels);
    exCntPtrs.resize(totalNumChannels);
    for (int ch = 0; ch < totalNumChannels; ch++) {
        inWtPtrs[ch] = 0;
        smpCnts[ch] = 0;
        outWtPtrs[ch] = HOPSIZE;
        outRdPtrs[ch] = 0;
        exPtrs[ch] = 0;
        histPtrs[ch] = 0;
        exCntPtrs[ch] = 0;
        inRdPtrs[ch] = 0;
    }
    inBuf.resize(totalNumChannels);
    outBuf.resize(totalNumChannels);
    out_hist.resize(totalNumChannels);
    scBuf.resize(totalNumChannels);
    for (int ch = 0; ch < totalNumChannels; ch++) {
        inBuf[ch].resize(BUFLEN);
        scBuf[ch].resize(BUFLEN);
        outBuf[ch].resize(BUFLEN);
        for (int i = 0; i < ORDER; i++) {
            out_hist[ch][i] = 0.0;
        }
    }
}

void LPC::applyLPC(const float *input, float *output, int numSamples, float lpcMix, float exPercentage, int ch, float exStartPos, const float *sidechain, float previousGain, float currentGain) {
    if (noise == nullptr) {
        return;
    }
    inWtPtr = inWtPtrs[ch];
    inRdPtr = inRdPtrs[ch];
    smpCnt = smpCnts[ch];
    outRdPtr = outRdPtrs[ch];
    outWtPtr = (outRdPtr+HOPSIZE)%BUFLEN;
    if (HOPSIZE < prevFrameLen/2) {
        for (int i = 0; i < prevFrameLen/2-HOPSIZE; i++) {
            int idx = outWtPtr+i;
            if (idx >= BUFLEN) {
                idx -= BUFLEN;
            }
            outBuf[ch][idx] = 0;
        }
    }
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
    float slope = (currentGain-previousGain)/numSamples;
    int validHopSize = min(HOPSIZE, prevFrameLen/2);
    for (int s = 0; s < numSamples; s++) {
        inBuf[ch][inWtPtr] = input[s];
        if (sidechain != nullptr) {
            scBuf[ch][inWtPtr] = sidechain[s];
        }
        inWtPtr++;
        if (inWtPtr >= BUFLEN) {
            inWtPtr = 0;
        }
        double out = outBuf[ch][outRdPtr];
//        double outDb = 10*log10(out*out);
//        double newGainDb = -40.f-outDb;
//        gainDb = smoothFactor*newGainDb+(1.0-smoothFactor)*gainDb;
//        out *= pow(10, gainDb/10.0);
        double in = inBuf[ch][(inRdPtr+BUFLEN-FRAMELEN)%BUFLEN];
        inRdPtr++;
        if (inRdPtr >= BUFLEN) {
            inRdPtr = 0;
        }
        float gainFactor = previousGain+slope*(float)s;
        output[s] = lpcMix*gainFactor*out+(1-lpcMix)*in;
        outBuf[ch][outRdPtr] = 0;
        outRdPtr++;
        if (outRdPtr >= BUFLEN) {
            outRdPtr = 0;
        }
        smpCnt++;
        if (smpCnt >= validHopSize) {
            smpCnt = 0;
            if (sidechain != nullptr) {
                for (int i = 0; i < FRAMELEN; i++) {
                    int inBufIdx = (inWtPtr+i-FRAMELEN+BUFLEN)%BUFLEN;
                    orderedInBuf[i] = window[i]*inBuf[ch][inBufIdx];
                    orderedScBuf[i] = scBuf[ch][inBufIdx];
                }
            }
            else {
                for (int i = 0; i < FRAMELEN; i++) {
                    int inBufIdx = (inWtPtr+i-FRAMELEN+BUFLEN)%BUFLEN;
                    orderedInBuf[i] = window[i]*inBuf[ch][inBufIdx];
                }
            }
            for (int lag = 0; lag < ORDER+1; lag++) {
                phi[lag] = autocorrelate(orderedInBuf, FRAMELEN, lag);
            }
            if (phi[0] != 0) {
                levinson_durbin();
                double G = phi[0];
                for (int k = 0; k < ORDER; k++) {
                    G -= alphas[k+1]*phi[k+1];
                }
                G = sqrt(G);//sqrt((double)(ORDER*FRAMELEN)));
//                double G = 1.0;
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
                for (int i = 0; i < out_hist[ch].size(); i++) {
                    out_hist[ch][i] = 0;
                }
                histPtrs[ch] = 0;
            }
            outWtPtr += HOPSIZE;
            if (outWtPtr >= BUFLEN) {
                outWtPtr -= BUFLEN;
            }
        }
    }
    inRdPtrs[ch] = inRdPtr;
    inWtPtrs[ch] = inWtPtr;
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
