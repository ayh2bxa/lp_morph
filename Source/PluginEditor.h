/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <cmath>

//==============================================================================
/**
*/
using namespace juce;
using namespace std;

class VoicemorphAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::ComboBox::Listener, public juce::Button::Listener
{
public:
    VoicemorphAudioProcessorEditor (VoicemorphAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~VoicemorphAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked(juce::Button *b) override;
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    VoicemorphAudioProcessor& audioProcessor;
    ComboBox excitationDropdown;
    juce::Slider lpcSlider;
    juce::Label lpcLabel;
    juce::Slider exLenSlider;
    juce::Label exLenLabel;
    juce::Slider exStartSlider;
    juce::Label exStartLabel;
    Slider wetGainSlider;
    Label wetGainLabel;
    Slider orderSlider;
    Label orderLabel;
    Slider frameDurSlider;
    Label frameDurLabel;
    juce::ToggleButton sidechainButton;
    juce::TextButton contactButton;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lpcMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> exLenAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> exStartAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> orderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> frameDurAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> useSidechainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoicemorphAudioProcessorEditor)
};
