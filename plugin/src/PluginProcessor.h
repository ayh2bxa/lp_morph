/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "lpc.h"
#include "agc.h"
#include "ParameterHelper.h"
#include <cmath>

//==============================================================================
/**
*/

using namespace juce;
using namespace std;

class VoicemorphAudioProcessor  : public juce::AudioProcessor, public ValueTree::Listener, public juce::Timer
{
public:
    //==============================================================================
    VoicemorphAudioProcessor();
    ~VoicemorphAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    LPC lpc;
    
    AudioProcessorValueTreeState apvts;
    void setUsingCustomExcitation(bool useCustom);
    vector<juce::File> getCustomExcitationFiles() const;
    void setCustomExcitation(int index);
    void loadCustomExcitations(const juce::File& selectedFile);
    
    vector<vector<double>> factoryExcitations;
    
    std::atomic<bool> hasAudioWarning{false};
    
    // for midi flushing to gui
    void timerCallback() override;
    
private:
    float previousGain = 0;
    float currentGain = 0;
    void loadFactoryExcitations();
    juce::File writeBinaryDataToTempFile(const void* data, int size, const juce::String& fileName);
    vector<vector<double>> customExcitations;
    vector<juce::File> customExcitationFiles;
    bool isUsingCustomExcitation() const;
    std::atomic<float>* exLenParameter  = nullptr;
    std::atomic<float>* gainParameter  = nullptr;
    std::atomic<float>* lpcMixParameter  = nullptr;
    std::atomic<float>* lpcOrderParameter  = nullptr;
    std::atomic<float>* lpcExStartParameter  = nullptr;
    std::atomic<float>* lpcExTypeParameter  = nullptr;
    std::atomic<float>* frameDurParameter  = nullptr;
    std::atomic<float>* useSidechainParameter  = nullptr;
    bool isStandalone;
    bool usingCustomExcitation = false;
    int currentCustomExcitationIndex = -1;
    void updateLpcParams();
    
    // midi handling
    // Map a (channel, CC) to an APVTS parameter ID
    struct MidiMap { int ch; int cc; juce::String paramID; };
    std::vector<MidiMap> midiMap;

    // Per-parameter target written by the audio thread (normalized 0..1)
    std::unordered_map<juce::String, std::atomic<float>> midiTarget;

    // Message-thread copies for host/UI updates
    std::unordered_map<juce::String, float>  lastSent;
    std::unordered_map<juce::String, bool>   inGesture;
    std::unordered_map<juce::String, double> lastTouch;
    void initMidi();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoicemorphAudioProcessor)
};
