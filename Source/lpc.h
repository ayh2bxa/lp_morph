#include <iostream>
#include <array>
#include <random>
#include <cmath>
#include <JuceHeader.h>

using namespace std;

class LPC {
private:
    int FRAMELEN = 442;
    int HOPSIZE = FRAMELEN/2;
    int BUFLEN = 2*FRAMELEN;
    int SAMPLERATE = 44100;
    int MAX_EXLEN = SAMPLERATE/6;
    vector<double> phi;
    vector<double> alphas;
    vector<double> reflections;
    int inPtr;
    int outWtPtr;
    int outRdPtr;
    int smpCnt;
    int exPtr;
    int histPtr;
    int exCntPtr;
    float max_amp;
    vector<vector<double>> inBuf;
    vector<double> orderedInBuf;
    vector<vector<double>> outBuf;
    vector<double> window;
    
    void levinson_durbin();
    double autocorrelate(const vector<double>& x, int lag);
    void reset_a();
    vector<vector<double>> out_hist;
    
    vector<int> inPtrs;
    vector<int> smpCnts;
    vector<int> outWtPtrs;
    vector<int> outRdPtrs;
    vector<int> exPtrs;
    vector<int> exCntPtrs;
    vector<int> histPtrs;

public:
    LPC(int numChannels);
    bool start = false;
    void applyLPC(float *inout, int numSamples, float lpcMix, float exPercentage, int ch, float exStartPos);
    void set_exlen(int val) {EXLEN = val;};
    int get_exlen() {return EXLEN;};
    int get_max_exlen() {return MAX_EXLEN;};
    const std::vector<double>* noise = nullptr;
    int EXLEN = MAX_EXLEN;
    int ORDER = 32;
    int exType = 0;
    bool orderChanged = false;
    bool exTypeChanged = false;
};
