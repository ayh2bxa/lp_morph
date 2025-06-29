#include "lpc.h"
#include "../src/AudioLogger.h"

LPC::LPC() {
    double maxFrameDurS = MAX_FRAME_DUR/1000.0;
    ORDER = MAX_ORDER;
    FRAMELEN = (int)(SAMPLERATE*maxFrameDurS);
    prevFrameLen = FRAMELEN;
    HOPSIZE = FRAMELEN/2;
    
    // Initialize single channel variables
    inWtPtr = 0;
    smpCnt = 0;
    outWtPtr = HOPSIZE;
    outRdPtr = 0;
    exPtr = 0;
    histPtr = 0;
    exCntPtr = 0;
    inRdPtr = 0;
    
    phi.resize(ORDER+1);
    alphas.resize(ORDER+1);
    orderedInBuf.resize(FRAMELEN);
    orderedScBuf.resize(FRAMELEN);
    window.resize(FRAMELEN);
    
    // Single channel buffers
    inBuf.resize(BUFLEN);
    scBuf.resize(BUFLEN);
    outBuf.resize(BUFLEN);
    out_hist.resize(ORDER);
    
    for (int i = 0; i < BUFLEN; i++) {
        inBuf[i] = 0.0;
        outBuf[i] = 0.0;
    }
    for (int i = 0; i < ORDER; i++) {
        out_hist[i] = 0.0;
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
double LPC::levinson_durbin() {
    reset_a();
    double E = phi[0];
    for (int k = 0; k < ORDER; k++) {
        double lbda = 0.0;
        for (int j = 0; j <= k; j++) {
            lbda += alphas[j] * phi[k + 1 - j];
        }
        lbda = -lbda / E;
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
    HOPSIZE = FRAMELEN/2;
    prevFrameLen = FRAMELEN;
    for (int i = 0; i < FRAMELEN; i++) {
        window[i] = 0.5*(1.0-cos(2.0*M_PI*i/(double)(FRAMELEN-1)));
    }
    
    // Reset single channel state
    inWtPtr = 0;
    smpCnt = 0;
    outWtPtr = HOPSIZE;
    outRdPtr = 0;
    exPtr = 0;
    histPtr = 0;
    exCntPtr = 0;
    inRdPtr = 0;
    
    // Resize buffers if needed
    inBuf.resize(BUFLEN);
    outBuf.resize(BUFLEN);
    out_hist.resize(MAX_ORDER);
}

LPCWarning LPC::applyLPC(const float *input, float *output, int numSamples, float lpcMix, float exPercentage, float exStartPos, const float *sidechain, float previousGain, float currentGain) {
    LPCWarning warningType = LPCWarning::None;
    if (noise == nullptr) {
        return warningType;
    }
    
    outWtPtr = (outRdPtr+HOPSIZE)%BUFLEN;
    int exStart = static_cast<int>(exStartPos*EXLEN);
    if (exTypeChanged) {
        exPtr = exStart;
        exCntPtr = 0;
        histPtr = 0;
    }
    if (orderChanged) {
        for (int i = 0; i < out_hist.size(); i++) {
            out_hist[i] = 0;
        }
        histPtr = 0;
    }
    double slope = (currentGain-previousGain)/numSamples;
    for (int s = 0; s < numSamples; s++) {
        inBuf[inWtPtr] = (double)input[s];
        if (sidechain != nullptr) {
            scBuf[inWtPtr] = sidechain[s];
        }
        inWtPtr++;
        if (inWtPtr >= BUFLEN) {
            inWtPtr = 0;
        }
        double out = outBuf[outRdPtr];
        double in = inBuf[(inRdPtr+BUFLEN-FRAMELEN)%BUFLEN];
        inRdPtr++;
        if (inRdPtr >= BUFLEN) {
            inRdPtr = 0;
        }
        double gainFactor = previousGain+slope*(double)s;
        float final_out = lpcMix*gainFactor*out+(1-lpcMix)*in;
        if (isnan(final_out)) {
            warningType = LPCWarning::NaN;
            final_out = 0.f;
        }
        else if (fabsf(final_out) > 1.f) {
            warningType = LPCWarning::Clipping;
            final_out /= (2.f*fabsf(final_out));
        }
        output[s] = final_out;
        outBuf[outRdPtr] = 0;
        outRdPtr++;
        if (outRdPtr >= BUFLEN) {
            outRdPtr = 0;
        }
        smpCnt++;
        if (smpCnt >= HOPSIZE) {
            smpCnt = 0;
            for (int i = 0; i < FRAMELEN; i++) {
                int inBufIdx = (inWtPtr+i-FRAMELEN+BUFLEN)%BUFLEN;
                orderedInBuf[i] = window[i]*inBuf[inBufIdx];
            }
            if (sidechain != nullptr) {
                for (int i = 0; i < FRAMELEN; i++) {
                    int inBufIdx = (inWtPtr+i-FRAMELEN+BUFLEN)%BUFLEN;
                    orderedScBuf[i] = scBuf[inBufIdx];
                }
            }
            for (int lag = 0; lag < ORDER+1; lag++) {
                phi[lag] = autocorrelate(orderedInBuf, FRAMELEN, lag);
            }
            if (phi[0] != 0) {
                double err = levinson_durbin();
                double G = sqrt(err);
                
                // Log alphas after levinson_durbin
                if (audioLogger) {
                    audioLogger->logPhis(phi.data(), ORDER);
                    audioLogger->logG(G, err);
                }
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
                        double out_n = G*ex;
                        for (int k = 0; k < ORDER; k++) {
                            int idx = (histPtr+k)%ORDER;
                            out_n -= alphas[k+1]*out_hist[idx];
                        }
                        histPtr--;
                        if (histPtr < 0) {
                            histPtr += ORDER;
                        }
                        out_hist[histPtr] = out_n;
                        unsigned long wtIdx = (outWtPtr+n)%BUFLEN;
                        // Change of frame length can cause OLA to add with unwanted audio
                        // in a correct scenario, OLA with 50% overlap will always be adding with 0s in
                        // the last HOPSIZE-many samples, so just set the last HOPSIZE-many outputs to
                        // be equal to out_n
                        if (n < HOPSIZE) {
                            outBuf[wtIdx] += out_n;
                        }
                        else {
                            outBuf[wtIdx] = out_n;
                        }
                    }
                }
                else {
                    for (int n = 0; n < FRAMELEN; n++) {
                        double ex = orderedScBuf[n];
                        double out_n = G*ex;
                        for (int k = 0; k < ORDER; k++) {
                            int idx = (histPtr+k)%ORDER;
                            out_n -= alphas[k+1]*out_hist[idx];
                        }
                        histPtr--;
                        if (histPtr < 0) {
                            histPtr += ORDER;
                        }
                        out_hist[histPtr] = out_n;
                        unsigned long wtIdx = (outWtPtr+n)%BUFLEN;
                        if (n < HOPSIZE) {
                            outBuf[wtIdx] += out_n;
                        }
                        else {
                            outBuf[wtIdx] = out_n;
                        }
                    }
                }
                for (int i = 0; i < out_hist.size(); i++) {
                    out_hist[i] = 0;
                }
                histPtr = 0;
            }
            outWtPtr += HOPSIZE;
            if (outWtPtr >= BUFLEN) {
                outWtPtr -= BUFLEN;
            }
        }
    }
    return warningType;
}

void LPC::reset_a() {
    alphas[0] = 1.0;
    for (int i = 1; i < MAX_ORDER+1; i++) {
        alphas[i] = 0.0;
    }
}
