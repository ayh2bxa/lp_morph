/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VoicemorphAudioProcessorEditor::VoicemorphAudioProcessorEditor (VoicemorphAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), spectrumImage (juce::Image::RGB, 512, 512, true)
{
    setOpaque (true);
    startTimerHz(30);
    
    lpcSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    lpcSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(lpcSlider);
    lpcLabel.setText("Mix", juce::dontSendNotification);
    lpcLabel.attachToComponent(&lpcSlider, false);
    addAndMakeVisible(lpcLabel);
    lpcMixAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (vts, "lpc mix", lpcSlider));
    
    exLenSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    exLenSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(exLenSlider);
    exLenLabel.setText("Length", juce::dontSendNotification);
    exLenLabel.attachToComponent(&exLenSlider, false);
    addAndMakeVisible(exLenLabel);
    exLenAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (vts, "ex len", exLenSlider));

    exStartSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    exStartSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(exStartSlider);
    exStartLabel.setText("Start", juce::dontSendNotification);
    exStartLabel.attachToComponent(&exStartSlider, false);
    addAndMakeVisible(exStartLabel);
    exStartAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (vts, "ex start pos", exStartSlider));
    
    orderSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    orderSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(orderSlider);
    orderLabel.setText("Order", juce::dontSendNotification);
    orderLabel.attachToComponent(&orderSlider, false);
    addAndMakeVisible(orderLabel);
    orderAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (vts, "lpc order", orderSlider));
    
    wetGainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    wetGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(wetGainSlider);
    wetGainLabel.setText("Wet gain (dB)", juce::dontSendNotification);
    wetGainLabel.attachToComponent(&wetGainSlider, false);
    addAndMakeVisible(wetGainLabel);
    wetGainAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (vts, "wet gain", wetGainSlider));
    
    frameDurSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    frameDurSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(frameDurSlider);
    frameDurLabel.setText("Frame Duration (ms)", juce::dontSendNotification);
    frameDurLabel.attachToComponent(&frameDurSlider, false);
    addAndMakeVisible(frameDurLabel);
    frameDurAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (vts, "frame dur", frameDurSlider));
    
    
    excitationDropdown.addItem("BassyTrainNoise", 1);
    excitationDropdown.addItem("CherubScreams", 2);
    excitationDropdown.addItem("MicScratch", 3);
    excitationDropdown.addItem("Ring", 4);
    excitationDropdown.addItem("TrainScreech1", 5);
    excitationDropdown.addItem("TrainScreech2", 6);
    excitationDropdown.addItem("WhiteNoise", 7);
    excitationDropdown.addItem("Off", 8);
    addAndMakeVisible(excitationDropdown);
    excitationDropdown.addListener(this);
    int exType = vts.getParameterAsValue("ex type").getValue();
    excitationDropdown.setSelectedId(1+exType, juce::dontSendNotification);
    
    customExcitationDropdown.addItem("No custom files loaded", 1);
    customExcitationDropdown.setEnabled(false);
    addAndMakeVisible(customExcitationDropdown);
    customExcitationDropdown.addListener(this);
    
    customButton.setButtonText("Custom");
    customButton.addListener(this);
    addAndMakeVisible(customButton);
    
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

    g.setColour (juce::Colours::white);
    auto inputLineColour = juce::Colours::white;
    auto estimatedFilterLineColour = juce::Colours::red;
    
}

void VoicemorphAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    lpcSlider.setBoundsRelative(0.04, 0.05, 0.28, 0.15);
    exLenSlider.setBoundsRelative(0.36, 0.05, 0.28, 0.15);
    exStartSlider.setBoundsRelative(0.68, 0.05, 0.28, 0.15);
    orderSlider.setBoundsRelative(0.04, 0.3, 0.28, 0.15);
    wetGainSlider.setBoundsRelative(0.36, 0.3, 0.28, 0.15);
    frameDurSlider.setBoundsRelative(0.68, 0.3, 0.28, 0.15);
    excitationDropdown.setBoundsRelative(0.04, 0.9, 0.28, 0.04);
    customExcitationDropdown.setBoundsRelative(0.36, 0.9, 0.28, 0.04);
    customButton.setBoundsRelative(0.68, 0.9, 0.28, 0.04);
    contactButton.setBoundsRelative(0.36, 0.95, 0.28, 0.04);
}

void VoicemorphAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &excitationDropdown) {
        int selectedId = excitationDropdown.getSelectedId();
        if (selectedId != 8) {
            audioProcessor.apvts.getParameterAsValue("ex type").setValue(selectedId-1);
            audioProcessor.setUsingCustomExcitation(false);
            if (customExcitationDropdown.isEnabled()) {
                auto customFiles = audioProcessor.getCustomExcitationFiles();
                customExcitationDropdown.setSelectedId(customFiles.size() + 1, juce::dontSendNotification);
            }
        } else {
            audioProcessor.apvts.getParameterAsValue("ex type").setValue(7);
            audioProcessor.setUsingCustomExcitation(false);
        }
    }
    else if (comboBoxThatHasChanged == &customExcitationDropdown) {
        auto customFiles = audioProcessor.getCustomExcitationFiles();
        int selectedId = customExcitationDropdown.getSelectedId();
        
        if (selectedId > 0 && selectedId <= customFiles.size()) {
            audioProcessor.setCustomExcitation(selectedId-1);
            audioProcessor.setUsingCustomExcitation(true);
            excitationDropdown.setSelectedId(8, juce::dontSendNotification);
        } else if (selectedId == customFiles.size() + 1) {
            audioProcessor.setUsingCustomExcitation(false);
            audioProcessor.apvts.getParameterAsValue("ex type").setValue(7);
        }
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
    else if (b == &customButton) {
        chooser = std::make_unique<juce::FileChooser> ("Select a WAV file", juce::File{}, "*.wav");
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        chooser->launchAsync(chooserFlags,
                               [this](const juce::FileChooser& fc)
                               {
            juce::File selectedFile = fc.getResult();
            if (selectedFile.existsAsFile()) {
                audioProcessor.loadCustomExcitations(selectedFile);
                updateCustomExcitationDropdown();
            }
        });
    }
}

void VoicemorphAudioProcessorEditor::timerCallback() {
    
}

void VoicemorphAudioProcessorEditor::updateCustomExcitationDropdown() {
    customExcitationDropdown.clear();
    auto customFiles = audioProcessor.getCustomExcitationFiles();
    
    if (customFiles.empty()) {
        customExcitationDropdown.addItem("No custom files loaded", 1);
        customExcitationDropdown.setEnabled(false);
    } else {
        for (int i = 0; i < customFiles.size(); ++i) {
            customExcitationDropdown.addItem(customFiles[i].getFileNameWithoutExtension(), i + 1);
        }
        customExcitationDropdown.addItem("Off", customFiles.size() + 1);
        customExcitationDropdown.setEnabled(true);
        customExcitationDropdown.setSelectedId(customFiles.size() + 1, juce::dontSendNotification);
    }
}
