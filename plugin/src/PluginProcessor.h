/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "lpc.h"
#include "ParameterHelper.h"
#include <cmath>

//==============================================================================
/**
*/

using namespace juce;
using namespace std;

class VoicemorphAudioProcessor  : public juce::AudioProcessor, public ValueTree::Listener
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
private:
    float previousGain = 0;
    float currentGain = 0;
    void loadFactoryExcitations();
    juce::File writeBinaryDataToTempFile(const void* data, int size, const juce::String& fileName);
    vector<vector<double>> factoryExcitations;
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
    bool isStandalone;
    bool usingCustomExcitation = false;
    int currentCustomExcitationIndex = -1;
    void updateLpcParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoicemorphAudioProcessor)
};
