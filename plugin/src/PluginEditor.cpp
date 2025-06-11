/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VoicemorphAudioProcessorEditor::VoicemorphAudioProcessorEditor (VoicemorphAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setOpaque (true);
    startTimerHz(30);
    
    lpcSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    lpcSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(lpcSlider);
    lpcLabel.setText("Mix", juce::dontSendNotification);
    lpcLabel.attachToComponent(&lpcSlider, false);
//    lpcLabel.setColour (juce::Label::textColourId, readingsColour);
    addAndMakeVisible(lpcLabel);
    lpcMixAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (vts, "lpcMix", lpcSlider));
    
    exLenSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    exLenSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(exLenSlider);
    exLenLabel.setText("Length", juce::dontSendNotification);
    exLenLabel.attachToComponent(&exLenSlider, false);
//    exLenLabel.setColour (juce::Label::textColourId, readingsColour);
    addAndMakeVisible(exLenLabel);
    exLenAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (vts, "exLen", exLenSlider));

    exStartSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    exStartSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(exStartSlider);
    exStartLabel.setText("Start", juce::dontSendNotification);
    exStartLabel.attachToComponent(&exStartSlider, false);
//    exStartLabel.setColour (juce::Label::textColourId, readingsColour);
    addAndMakeVisible(exStartLabel);
    exStartAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (vts, "exStartPos", exStartSlider));
    
    orderSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    orderSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(orderSlider);
    orderLabel.setText("Order", juce::dontSendNotification);
    orderLabel.attachToComponent(&orderSlider, false);
//    orderLabel.setColour (juce::Label::textColourId, readingsColour);
    addAndMakeVisible(orderLabel);
    orderAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (vts, "lpcOrder", orderSlider));
    
    wetGainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    wetGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(wetGainSlider);
    wetGainLabel.setText("Wet gain (dB)", juce::dontSendNotification);
    wetGainLabel.attachToComponent(&wetGainSlider, false);
//    wetGainLabel.setColour (juce::Label::textColourId, readingsColour);
    addAndMakeVisible(wetGainLabel);
    wetGainAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (vts, "wetGain", wetGainSlider));
    
    frameDurSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    frameDurSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(frameDurSlider);
    frameDurLabel.setText("Frame Duration (ms)", juce::dontSendNotification);
    frameDurLabel.attachToComponent(&frameDurSlider, false);
//    frameDurLabel.setColour (juce::Label::textColourId, readingsColour);
    addAndMakeVisible(frameDurLabel);
    frameDurAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (vts, "frameDur", frameDurSlider));
    
    sidechainButton.setButtonText ("Sidechain");
//    sidechainButton.setColour (juce::Label::textColourId, readingsColour);
    addAndMakeVisible(sidechainButton);
    sidechainButton.setVisible(!JUCEApplication::isStandaloneApp());
    sidechainButton.addListener(this);
    exLenSlider.setVisible(!sidechainButton.getToggleState());
    exStartSlider.setVisible(!sidechainButton.getToggleState());
    useSidechainAttachment.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (vts, "useSidechain", sidechainButton));
    
    excitationDropdown.addItem("BassyTrainNoise", 1);
    excitationDropdown.addItem("CherubScreams", 2);
    excitationDropdown.addItem("MicScratch", 3);
    excitationDropdown.addItem("Ring", 4);
    excitationDropdown.addItem("TrainScreech1", 5);
    excitationDropdown.addItem("TrainScreech2", 6);
    excitationDropdown.addItem("WhiteNoise", 7);
//    excitationDropdown.setColour (juce::Label::textColourId, bgColour);
    addAndMakeVisible(excitationDropdown);
    excitationDropdown.addListener(this);
    int exType = vts.getParameterAsValue("exType").getValue();
    excitationDropdown.setSelectedId(1+exType, juce::dontSendNotification);
    
    contactButton.setButtonText("Contact Author :-)))");
    contactButton.addListener(this);
    addAndMakeVisible(contactButton);
    
    setSize (600, 600);
    setResizable(true, true);
}

VoicemorphAudioProcessorEditor::~VoicemorphAudioProcessorEditor()
{
    excitationDropdown.removeListener(this);
}

//==============================================================================
void VoicemorphAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
//    g.fillAll (bgColour);
    g.setColour (juce::Colours::white);
    auto inputLineColour = juce::Colours::white;
    auto estimatedFilterLineColour = juce::Colours::red;
    
}

void VoicemorphAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    lpcSlider.setBoundsRelative(0.04, 0.1, 0.28, 0.3);
    exLenSlider.setBoundsRelative(0.36, 0.1, 0.28, 0.3);
    exStartSlider.setBoundsRelative(0.68, 0.1, 0.28, 0.3);
    orderSlider.setBoundsRelative(0.04, 0.5, 0.28, 0.3);
    wetGainSlider.setBoundsRelative(0.36, 0.5, 0.28, 0.3);
    frameDurSlider.setBoundsRelative(0.68, 0.5, 0.28, 0.3);
    excitationDropdown.setBoundsRelative(0.04, 0.9, 0.28, 0.05);
//    sidechainButton.setBoundsRelative(0.36, 0.9, 0.28, 0.05);
    contactButton.setBoundsRelative(0.68, 0.9, 0.28, 0.05);
}

void VoicemorphAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &excitationDropdown) {
        audioProcessor.apvts.getParameterAsValue("exType").setValue(excitationDropdown.getSelectedId()-1);
    }
}

void VoicemorphAudioProcessorEditor::buttonClicked(juce::Button* b)
{
    if (b == &contactButton) {
        juce::URL url ("https://linktr.ee/projectfmusic");
        if (url.isWellFormed()) {
            url.launchInDefaultBrowser();
        }
    }
    else if (b == &sidechainButton) {
        exLenSlider.setVisible(!b->getToggleState());
        exStartSlider.setVisible(!b->getToggleState());
    }
}

void VoicemorphAudioProcessorEditor::timerCallback() {
    
}
