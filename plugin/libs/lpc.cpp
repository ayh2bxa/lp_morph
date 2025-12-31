#include "lpc.h"
#include <fstream>

LPC::LPC(int numChannels) {
    double maxFrameDurS = MAX_FRAME_DUR/1000.0;
    ORDER = MAX_ORDER;
    FRAMELEN = (int)(SAMPLERATE*maxFrameDurS);
    prevFrameLen = FRAMELEN;
    HOPSIZE = FRAMELEN/2;
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
    reflectionCoeffs.resize(ORDER);
    orderedInBuf.resize(FRAMELEN);
    orderedScBuf.resize(FRAMELEN);
    window.resize(FRAMELEN);
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
    DBG("DEBUGGING MODE");
}

//http://www.emptyloop.com/technotes/A%20tutorial%20on%20linear%20prediction%20and%20Levinson-Durbin.pdf
double LPC::levinson_durbin() {
    reset_a();
    double E = phi[0];
    for (int k = 0; k < ORDER; k++) {
        double lbda = 0.0;
        for (int j = 0; j <= k; j++) {
            lbda += alphas[j] * phi[k + 1 - j];
        }
        lbda = -lbda / E;
        reflectionCoeffs[k] = lbda;  // Store reflection coefficient
        // Clamp reflection coefficient for stability
        if (reflectionCoeffs[k] >= 1.0) reflectionCoeffs[k] = 0.999;
        if (reflectionCoeffs[k] <= -1.0) reflectionCoeffs[k] = -0.999;
        int half = (k + 1) / 2;
        for (int n = 0; n <= half; n++) {
            double tmp = alphas[k + 1 - n] + lbda * alphas[n];
            alphas[n] += lbda * alphas[k + 1 - n];
            alphas[k + 1 - n] = tmp;
        }
        E *= (1.0 - lbda * lbda);
    }
    return E;
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
    HOPSIZE = FRAMELEN/2;
    prevFrameLen = FRAMELEN;
    for (int i = 0; i < FRAMELEN; i++) {
        window[i] = 0.5*(1.0-cos(2.0*M_PI*i/(double)(FRAMELEN-1)));
    }
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
    for (int ch = 0; ch < totalNumChannels; ch++) {
        inBuf[ch].resize(BUFLEN);
        outBuf[ch].resize(BUFLEN);
        out_hist[ch].resize(MAX_ORDER);
    }
}

bool LPC::applyLPC(const float *input, float *output, int numSamples, float lpcMix, float exPercentage, int ch, float exStartPos, const float *sidechain, float previousGain, float currentGain) {
    bool audioWarning = false;
    if (noise == nullptr) {
        return audioWarning;
    }
    inWtPtr = inWtPtrs[ch];
    inRdPtr = inRdPtrs[ch];
    smpCnt = smpCnts[ch];
    outRdPtr = outRdPtrs[ch];
    outWtPtr = (outRdPtr+HOPSIZE)%BUFLEN;
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
    double slope = (currentGain-previousGain)/numSamples;
    for (int s = 0; s < numSamples; s++) {
        inBuf[ch][inWtPtr] = (double)input[s];
        if (sidechain != nullptr) {
            scBuf[ch][inWtPtr] = sidechain[s];
        }
        inWtPtr++;
        if (inWtPtr >= BUFLEN) {
            inWtPtr = 0;
        }
        double out = outBuf[ch][outRdPtr];
        double in = inBuf[ch][(inRdPtr+BUFLEN-FRAMELEN)%BUFLEN];
        inRdPtr++;
        if (inRdPtr >= BUFLEN) {
            inRdPtr = 0;
        }
        double gainFactor = previousGain+slope*(double)s;
        float final_out = lpcMix*gainFactor*out+(1-lpcMix)*in;
        if (isnan(final_out)) {
            audioWarning = true;
            final_out = 0.f;
        }
        else if (fabsf(final_out) > 1.f) {
            audioWarning = true;
            final_out /= (2.f*fabsf(final_out));
        }
        output[s] = final_out;
        outBuf[ch][outRdPtr] = 0;
        outRdPtr++;
        if (outRdPtr >= BUFLEN) {
            outRdPtr = 0;
        }
        smpCnt++;
        if (smpCnt >= HOPSIZE) {
            smpCnt = 0;
            for (int i = 0; i < FRAMELEN; i++) {
                int inBufIdx = (inWtPtr+i-FRAMELEN+BUFLEN)%BUFLEN;
                orderedInBuf[i] = window[i]*inBuf[ch][inBufIdx];
            }
            if (sidechain != nullptr) {
                for (int i = 0; i < FRAMELEN; i++) {
                    int inBufIdx = (inWtPtr+i-FRAMELEN+BUFLEN)%BUFLEN;
                    orderedScBuf[i] = scBuf[ch][inBufIdx];
                }
            }
            for (int lag = 0; lag < ORDER+1; lag++) {
                phi[lag] = autocorrelate(orderedInBuf, FRAMELEN, lag);
            }
            if (phi[0] != 0) {
                double G = sqrt(levinson_durbin());
                if (sidechain == nullptr) {
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
                        // Lattice filter synthesis
                        double f = G * ex;
                        for (int i = ORDER - 1; i >= 0; --i) {
                            const double ki = -reflectionCoeffs[i];
                            const double bPrev = out_hist[ch][i];      // ẽ^(i-1)[n-1] - backward delay state

                            // Equation 11.100b: e^(i-1)[n] = e^(i)[n] + k_i * ẽ^(i-1)[n-1]
                            const double fPrev = f + ki * bPrev;
                            // Equation 11.100c: ẽ^(i)[n] = ẽ^(i-1)[n-1] - k_i * e^(i-1)[n]
                            const double bNew = bPrev - ki * fPrev;

                            out_hist[ch][i] = bNew;
                            f = fPrev;
                        }
                        double out_n = f;
                        unsigned long wtIdx = (outWtPtr+n)%BUFLEN;
                        double preOlaVal = outBuf[ch][wtIdx];
                        // Change of frame length can cause OLA to add with unwanted audio
                        // in a correct scenario, OLA with 50% overlap will always be adding with 0s in
                        // the last HOPSIZE-many samples, so just set the last HOPSIZE-many outputs to
                        // be equal to out_n
                        if (n < HOPSIZE) {
                            outBuf[ch][wtIdx] += out_n;
                        }
                        else {
                            outBuf[ch][wtIdx] = out_n;
                        }
                    }
                }
                else {
                    for (int n = 0; n < FRAMELEN; n++) {
                        double ex = orderedScBuf[n];
                        // Lattice filter synthesis
                        double f = G * ex;
                        for (int i = ORDER - 1; i >= 0; --i) {
                            const double ki = -reflectionCoeffs[i];
                            const double bPrev = out_hist[ch][i];      // ẽ^(i-1)[n-1] - backward delay state

                            // Equation 11.100b: e^(i-1)[n] = e^(i)[n] + k_i * ẽ^(i-1)[n-1]
                            const double fPrev = f + ki * bPrev;
                            // Equation 11.100c: ẽ^(i)[n] = ẽ^(i-1)[n-1] - k_i * e^(i-1)[n]
                            const double bNew = bPrev - ki * fPrev;

                            out_hist[ch][i] = bNew;
                            f = fPrev;
                        }
                        double out_n = f;
                        unsigned long wtIdx = (outWtPtr+n)%BUFLEN;
                        double preOlaVal = outBuf[ch][wtIdx];
                        if (n < HOPSIZE) {
                            outBuf[ch][wtIdx] += out_n;
                        }
                        else {
                            outBuf[ch][wtIdx] = out_n;
                        }
                    }
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
    exCntPtrs[ch] = exCntPtr;
    histPtrs[ch] = histPtr;
    return audioWarning;
}

void LPC::reset_a() {
    alphas[0] = 1.0;
    for (int i = 1; i < ORDER+1; i++) {
        alphas[i] = 0.0;
    }
}
