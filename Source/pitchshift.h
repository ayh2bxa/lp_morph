#pragma once

#include <cmath>
#include <vector>
#include <iostream>
#include <random>
#include "signalsmith-fft.h"

class PhaseVocoder {
public:
    PhaseVocoder(int fftSize, int bufferSize, float ampFactor, float pitchFactor, int numChannels);
    void applyPV(float *data, int numSamples, int channel);
private:
    std::vector<int> inPtr;
    std::vector<int> outWtPtr;
    std::vector<int> outRdPtr;
    int bufLen;
    std::vector<float> window;
    std::vector<std::vector<float>> inBuf;
    std::vector<std::vector<float>> outBuf;
    std::vector<std::vector<float>> prevPhase;
    std::vector<std::vector<float>> outPrevPhase;
    float ampScale;
    int fftPoints;
    int hopSize;
    float pitchShift;
    int smpCnt;
    std::vector<std::vector<float>> orderedInBuf;
    std::vector<std::vector<std::complex<float>>> spectrum;
    std::vector<std::vector<float>> td;
    signalsmith::RealFFT<float> fft;
    int channelCnt;
    std::vector<float> binFreq;
    std::vector<std::vector<float>> trueBin;
    std::vector<std::vector<float>> trueMag;
    std::vector<std::vector<float>> newBin;
    std::vector<std::vector<float>> newMag;
    float twoPiH;
};
