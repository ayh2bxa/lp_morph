#include <JuceHeader.h>
#include "../src/ColorScheme.h"

class WaveformViewer : public juce::Component
{
public:
    WaveformViewer();
    ~WaveformViewer() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setWaveform(const std::vector<double>* waveform);
    void clearWaveform();
    void setPlayheadPosition(float startPos, float currentPos);
    
private:
    const std::vector<double>* currentWaveform;
    juce::Path waveformPath;
    void generateWaveformPath();
    
    float playheadStartPos;
    float playheadCurrentPos;
    bool showPlayhead;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformViewer)
};

class Sampler {
private:
    
public:
};
