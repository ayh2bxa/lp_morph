#include <iostream>
#include <array>
#include <random>
#include <cmath>
#include <JuceHeader.h>

using namespace std;

class LPC {
private:
    vector<double> phi;
    vector<double> alphas;
    vector<double> reflections;
    int inWtPtr;
    int outWtPtr;
    int outRdPtr;
    int smpCnt;
    int exPtr;
    int histPtr;
    int exCntPtr;
    size_t inRdPtr;
    vector<vector<double>> inBuf;
    vector<vector<double>> scBuf;
    vector<double> orderedInBuf;
    vector<double> orderedScBuf;
    vector<vector<double>> outBuf;
    
    void levinson_durbin();
    double autocorrelate(const vector<double>& x, int frameSize, int lag);
    void reset_a();
    vector<vector<double>> out_hist;
    
    vector<int> inWtPtrs;
    vector<size_t> inRdPtrs;
    vector<int> smpCnts;
    vector<int> outWtPtrs;
    vector<int> outRdPtrs;
    vector<int> exPtrs;
    vector<int> exCntPtrs;
    vector<int> histPtrs;
    int totalNumChannels;
public:
    LPC(int numChannels);
    bool start = false;
    void applyLPC(const float *input, float *output, int numSamples, float lpcMix, float exPercentage, int ch, float exStartPos, const float *sidechain, float previousGain, float currentGain);
    void set_exlen(int val) {EXLEN = val;};
    int get_exlen() {return EXLEN;};
    int get_max_exlen() {return MAX_EXLEN;};
    const std::vector<double>* noise = nullptr;
    int FRAMELEN = 441;
    int prevFrameLen = FRAMELEN;
    int HOPSIZE = FRAMELEN/2;
    int BUFLEN = 16384;
    int SAMPLERATE = 44100;
    int MAX_EXLEN = SAMPLERATE/6;
    int EXLEN = MAX_EXLEN;
    int ORDER = 32;
    int exType = 0;
    bool orderChanged = false;
    bool exTypeChanged = false;
    bool exStartChanged = false;
    float exStart = 0.f;
    vector<double> window;
    void prepareToPlay();
};
