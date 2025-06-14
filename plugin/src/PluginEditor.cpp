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
    
    initialiseSlider(lpcSlider, lpcLabel, lpcMixAttachment, vts, "lpcMix", "Mix");
    initialiseSlider(exLenSlider, exLenLabel, exLenAttachment, vts, "exLen", "Length");
    initialiseSlider(exStartSlider, exStartLabel, exStartAttachment, vts, "exStartPos", "Start");
    initialiseSlider(orderSlider, orderLabel, orderAttachment, vts, "lpcOrder", "Order");
    initialiseSlider(wetGainSlider, wetGainLabel, wetGainAttachment, vts, "wetGain", "Wet gain (dB)");
    initialiseSlider(frameDurSlider, frameDurLabel, frameDurAttachment, vts, "frameDur", "Frame Duration (ms)");
    
    sidechainButton.setButtonText ("Sidechain");
    sidechainButton.setColour (juce::Label::textColourId, readingsColour);
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
    excitationDropdown.setColour(juce::ComboBox::backgroundColourId, juce::Colours::black);
    excitationDropdown.setColour(juce::ComboBox::textColourId, bgColour);
    getLookAndFeel().setColour(juce::PopupMenu::backgroundColourId, juce::Colours::black);
    getLookAndFeel().setColour(juce::PopupMenu::textColourId, bgColour);
    getLookAndFeel().setColour(juce::PopupMenu::highlightedBackgroundColourId, highlightColour);
    getLookAndFeel().setColour(juce::PopupMenu::highlightedTextColourId, bgColour);
    addAndMakeVisible(excitationDropdown);
    excitationDropdown.addListener(this);
    int exType = vts.getParameterAsValue("exType").getValue();
    excitationDropdown.setSelectedId(1+exType, juce::dontSendNotification);
    
    contactButton.setButtonText("Contact Author :-)))");
    contactButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black);
    contactButton.setColour(juce::TextButton::textColourOffId, bgColour);
    contactButton.setColour(juce::TextButton::textColourOnId, bgColour);
    contactButton.addListener(this);
    addAndMakeVisible(contactButton);
    
    setSize (1000, 800);
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
//    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.fillAll (bgColour);
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

void VoicemorphAudioProcessorEditor::initialiseSlider(juce::Slider& slider, juce::Label& label, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment, juce::AudioProcessorValueTreeState& vts, const juce::String& parameterID, const juce::String& labelText)
{
    slider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    slider.setColour(juce::Slider::textBoxTextColourId, readingsColour);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::black);
    slider.setColour(juce::Slider::thumbColourId, highlightColour);
    addAndMakeVisible(slider);
    label.setText(labelText, juce::dontSendNotification);
    label.attachToComponent(&slider, false);
    label.setColour(juce::Label::textColourId, readingsColour);
    addAndMakeVisible(label);
    attachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, parameterID, slider));
}
