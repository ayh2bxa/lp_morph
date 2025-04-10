#include "pitchshift.h"

PhaseVocoder::PhaseVocoder(int fftSize, int bufferSize, float ampFactor, float pitchFactor, int numChannels): fft(fftSize) {
    fftPoints = fftSize;
    bufLen = bufferSize;
    channelCnt = numChannels;
    window.resize(fftPoints);
    for (int i = 0; i < fftPoints; i++) {
        window[i] = 0.5f*(1.0f-cosf(2.0*M_PI*i/(float)(fftPoints-1)));
    }
    inPtr.resize(numChannels);
    outRdPtr.resize(numChannels);
    outWtPtr.resize(numChannels);
    hopSize = fftSize/8;
    for (int c = 0; c < numChannels; c++) {
        outWtPtr[c] = fftPoints+2*hopSize;
    }
    ampScale = ampFactor;
    inBuf.resize(numChannels, std::vector<float>(bufLen));
    outBuf.resize(numChannels, std::vector<float>(bufLen));
    prevPhase.resize(numChannels, std::vector<float>(fftPoints/2+1));
    pitchShift = pitchFactor;
    smpCnt = 0;
    orderedInBuf.resize(numChannels, std::vector<float>(fftPoints));
    spectrum.resize(numChannels, std::vector<std::complex<float>>(fftPoints/2));
    td.resize(numChannels, std::vector<float>(fftPoints));
    binFreq.resize(fftPoints/2+1);
    for (int k = 0; k < fftPoints/2+1; k++) {
        binFreq[k] = (2.0*M_PI*(float)k)/(float)fftPoints;
    }
    trueBin.resize(numChannels, std::vector<float>(fftPoints/2+1));
    newBin.resize(numChannels, std::vector<float>(fftPoints/2+1));
    trueMag.resize(numChannels, std::vector<float>(fftPoints/2+1));
    newMag.resize(numChannels, std::vector<float>(fftPoints/2+1));
    outPrevPhase.resize(numChannels, std::vector<float>(fftPoints/2+1));
    twoPiH = 2.0*M_PI*(float)hopSize;
}

float toPA(float phase) {
    return phase < 0 ? fmodf(phase-M_PI, 2.0*M_PI)+M_PI : fmodf(phase+M_PI, 2.0*M_PI)-M_PI;
}

void PhaseVocoder::applyPV(float *data, int numSamples, int channel) {
    for (int s = 0; s < numSamples; s++) {
        // load audio sample into input buffer
        inBuf[channel][inPtr[channel]] = data[s];
        inPtr[channel]++;
        if (inPtr[channel] >= bufLen) {
            inPtr[channel] = 0;
        }
        // send output to audio buffer
        data[s] = ampScale*outBuf[channel][outRdPtr[channel]]/(float)fftPoints;
        outBuf[channel][outRdPtr[channel]] = 0.0f;
        outRdPtr[channel]++;
        if (outRdPtr[channel] >= bufLen) {
            outRdPtr[channel] = 0;
        }
        smpCnt++;
        if (smpCnt >= hopSize) { // when a hop is encountered, perform a pitch shift
            smpCnt = 0;
            // reorder the input buffer and multiply input by window
            for (int i = 0; i < fftPoints; i++) {
                orderedInBuf[channel][i] = window[i]*inBuf[channel][(i+inPtr[channel]-fftPoints+bufLen)%bufLen];
            }
            fft.fft(orderedInBuf[channel], spectrum[channel]);
            float binVal0 = spectrum[channel][0].real();
            trueMag[channel][0] = std::abs(binVal0);
            float phase0 = atan2(0.0, binVal0);
            float targetPhase0 = binFreq[0]*(float)hopSize + prevPhase[channel][0];
            float deviation0 = (float)fftPoints*toPA(phase0-targetPhase0)/twoPiH;
            prevPhase[channel][0] = phase0;
            trueBin[channel][0] = deviation0;
            for (int k = 1; k < fftPoints/2; k++) {
                float binMag = std::abs(spectrum[channel][k]);
                float phase = atan2(spectrum[channel][k].imag(), spectrum[channel][k].real());
                trueMag[channel][k] = binMag;
                float targetPhase = binFreq[k]*(float)hopSize + prevPhase[channel][k];
                float deviation = (float)fftPoints*toPA(phase-targetPhase)/twoPiH;
                prevPhase[channel][k] = phase;
                trueBin[channel][k] = deviation + (float)k;
            }
            float binValN = spectrum[channel][0].imag();
            trueMag[channel][fftPoints/2] = std::abs(binValN);
            float phaseN = atan2(0.0, binValN);
            float targetPhaseN = binFreq[fftPoints/2]*(float)hopSize + prevPhase[channel][fftPoints/2];
            float deviationN = (float)fftPoints*toPA(phaseN-targetPhaseN)/twoPiH;
            prevPhase[channel][fftPoints/2] = phaseN;
            trueBin[channel][fftPoints/2] = deviationN + (float)(fftPoints/2);
            for (int k = 0; k < fftPoints/2+1; k++) {
                newMag[channel][k] = 0;
                newBin[channel][k] = 0;
            }
            for (int k = 0; k < fftPoints/2+1; k++) {
                int binNum = floorf((float)k*pitchShift+0.5f);
                if (binNum < fftPoints/2+1) {
                    newMag[channel][binNum] += trueMag[channel][k];
                    newBin[channel][binNum] = trueBin[channel][k]*pitchShift;
                }
            }
            float phaseDeviation0 = binFreq[0]*(float)hopSize+twoPiH*(newBin[channel][0])/(float)fftPoints;
            float phase0s = toPA(outPrevPhase[channel][0]+phaseDeviation0);
            outPrevPhase[channel][0] = phase0s;
            spectrum[channel][0].real(newMag[channel][0]*cosf(phase0s));
            for (int k = 1; k < fftPoints/2; k++) {
                float phaseDeviation = binFreq[k]*(float)hopSize+twoPiH*(newBin[channel][k]-k)/(float)fftPoints;
                float recoveredPhase = outPrevPhase[channel][k]+phaseDeviation;
                float phase = toPA(recoveredPhase);
                outPrevPhase[channel][k] = phase;
                spectrum[channel][k].real(newMag[channel][k]*cosf(phase));
                spectrum[channel][k].imag(newMag[channel][k]*sinf(phase));
            }
            float phaseDeviationN = binFreq[fftPoints/2]*(float)hopSize+twoPiH*(newBin[channel][fftPoints/2]-fftPoints/2)/(float)fftPoints;
            float phaseNs = toPA(outPrevPhase[channel][fftPoints/2]+phaseDeviationN);
            outPrevPhase[channel][fftPoints/2] = phaseNs;
            spectrum[channel][0].imag(newMag[channel][fftPoints/2]*cosf(phaseNs));
            fft.ifft(spectrum[channel], td[channel]);
            for (int i = 0; i < fftPoints; i++) {
                outBuf[channel][(i+outWtPtr[channel])%bufLen] += window[i]*(td[channel][i]);
            }
            outWtPtr[channel] = (outWtPtr[channel]+hopSize)%bufLen;
        }
    }
}
