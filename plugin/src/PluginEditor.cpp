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
    sidechainButton.setColour (juce::Label::textColourId, ColorScheme::readingsColour);
    addAndMakeVisible(sidechainButton);
    sidechainButton.setVisible(!JUCEApplication::isStandaloneApp());
    sidechainButton.addListener(this);
    exLenSlider.setVisible(!sidechainButton.getToggleState());
    exStartSlider.setVisible(!sidechainButton.getToggleState());
    useSidechainAttachment.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (vts, "useSidechain", sidechainButton));
    
    for (int i = 0; i < excitationNames.size(); ++i) {
        excitationDropdown.addItem(excitationNames[i], i + 1);
    }
    excitationDropdown.setColour(juce::ComboBox::backgroundColourId, juce::Colours::black);
    excitationDropdown.setColour(juce::ComboBox::textColourId, ColorScheme::bgColour);
    getLookAndFeel().setColour(juce::PopupMenu::backgroundColourId, juce::Colours::black);
    getLookAndFeel().setColour(juce::PopupMenu::textColourId, ColorScheme::bgColour);
    getLookAndFeel().setColour(juce::PopupMenu::highlightedBackgroundColourId, ColorScheme::highlightColour);
    getLookAndFeel().setColour(juce::PopupMenu::highlightedTextColourId, ColorScheme::bgColour);
    addAndMakeVisible(excitationDropdown);
    excitationDropdown.addListener(this);
    int exType = vts.getParameterAsValue("exType").getValue();
    excitationDropdown.setSelectedId(1+exType, juce::dontSendNotification);
    
    settingsButton.setButtonText("Audio Settings");
    settingsButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black);
    settingsButton.setColour(juce::TextButton::textColourOffId, ColorScheme::bgColour);
    settingsButton.setColour(juce::TextButton::textColourOnId, ColorScheme::bgColour);
    settingsButton.addListener(this);
    addAndMakeVisible(settingsButton);
    
    addAndMakeVisible(waveformViewer);
    updateWaveformDisplay();
    
    showWarningIndicator = false;
    lastWarningTime = juce::Time::getCurrentTime();
    
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
    g.fillAll (ColorScheme::bgColour);
    g.setColour (juce::Colours::white);
    auto inputLineColour = juce::Colours::white;
    auto estimatedFilterLineColour = juce::Colours::red;
    
//    if (showWarningIndicator) {
//        g.setColour(ColorScheme::readingsColour);
//        g.setFont(juce::Font(24.0f, juce::Font::bold));
//        juce::Rectangle<int> warningRect(getWidth() - 40, 10, 30, 30);
//        g.drawText("!", warningRect, juce::Justification::centred);
//    }
    
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
    waveformViewer.setBoundsRelative(0.04, 0.82, 0.6, 0.15);
    excitationDropdown.setBoundsRelative(0.68, 0.82, 0.28, 0.05);
//    sidechainButton.setBoundsRelative(0.36, 0.9, 0.28, 0.05);
    settingsButton.setBoundsRelative(0.68, 0.92, 0.28, 0.05);
}

void VoicemorphAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &excitationDropdown) {
        audioProcessor.apvts.getParameterAsValue("exType").setValue(excitationDropdown.getSelectedId()-1);
        updateWaveformDisplay();
    }
}

void VoicemorphAudioProcessorEditor::buttonClicked(juce::Button* b)
{
    if (b == &settingsButton) {
//        juce::URL url ("https://linktr.ee/projectfmusic");
//        if (url.isWellFormed()) {
//            url.launchInDefaultBrowser();
//        }
        // Access the StandalonePluginHolder
        StandalonePluginHolder* pluginHolder = juce::StandalonePluginHolder::getInstance();

        // Get the AudioDeviceManager
        auto& deviceManager = pluginHolder->deviceManager;

        // Create the AudioDeviceSelectorComponent
        auto* audioSettingsComp = new juce::AudioDeviceSelectorComponent(
            deviceManager,
            0, 4,  // Min/max input channels
            0, 4,  // Min/max output channels
            false,  // Show input channels
            false,  // Show output channels
            false,  // Show sample rate selector
            false
        );
        audioSettingsComp->setSize(500, 550);
        // Create a dialog window to display the settings
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(audioSettingsComp); // Attach the settings component
        options.dialogTitle = TRANS("Settings"); // Set dialog title
        juce::Colour bgColour((uint8)0x05, (uint8)0x05, (uint8)0x05, (float)1.0f);
        options.dialogBackgroundColour = bgColour;
        options.escapeKeyTriggersCloseButton = true; // Close on escape key
        options.useNativeTitleBar = true; // Use native title bar
        options.resizable = false; // Non-resizable dialog

        // Launch the dialog asynchronously
        options.launchAsync();
    }
    else if (b == &sidechainButton) {
        exLenSlider.setVisible(!b->getToggleState());
        exStartSlider.setVisible(!b->getToggleState());
    }
}

void VoicemorphAudioProcessorEditor::timerCallback() {
    if (audioProcessor.factoryExcitations.size() > 0)
    {
        int selectedExcitation = excitationDropdown.getSelectedId() - 1;
        if (selectedExcitation >= 0 && selectedExcitation < static_cast<int>(audioProcessor.factoryExcitations.size()))
        {
            float exStart = audioProcessor.apvts.getParameterAsValue("exStartPos").getValue();
            int currentExPtr = audioProcessor.lpcChannels[0].getCurrentExPtr();
            
            float startPosInSamples = exStart * audioProcessor.factoryExcitations[selectedExcitation][0].size();
            
            waveformViewer.setPlayheadPosition(startPosInSamples, static_cast<float>(currentExPtr));
        }
    }
    
    bool hasWarning = audioProcessor.hasAudioWarning.load();
    if (hasWarning) {
        if (!showWarningIndicator) {
            showWarningIndicator = true;
            repaint();
        }
        lastWarningTime = juce::Time::getCurrentTime();
        audioProcessor.hasAudioWarning.store(false);
    } else {
        if (showWarningIndicator) {
            juce::Time currentTime = juce::Time::getCurrentTime();
            if ((currentTime - lastWarningTime).inMilliseconds() >= 2000) {
                showWarningIndicator = false;
                repaint();
            }
        }
    }
}

void VoicemorphAudioProcessorEditor::initialiseSlider(juce::Slider& slider, juce::Label& label, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment, juce::AudioProcessorValueTreeState& vts, const juce::String& parameterID, const juce::String& labelText)
{
    slider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    slider.setColour(juce::Slider::textBoxTextColourId, ColorScheme::readingsColour);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::black);
    slider.setColour(juce::Slider::thumbColourId, ColorScheme::highlightColour);
    addAndMakeVisible(slider);
    label.setText(labelText, juce::dontSendNotification);
    label.attachToComponent(&slider, false);
    label.setColour(juce::Label::textColourId, ColorScheme::readingsColour);
    addAndMakeVisible(label);
    attachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, parameterID, slider));
}

void VoicemorphAudioProcessorEditor::updateWaveformDisplay()
{
    int selectedExcitation = excitationDropdown.getSelectedId() - 1;
    
    if (selectedExcitation >= 0 && selectedExcitation < static_cast<int>(audioProcessor.factoryExcitations.size()))
    {
        waveformViewer.setWaveform(&audioProcessor.factoryExcitations[selectedExcitation][0]);

        float exStart = audioProcessor.apvts.getParameterAsValue("exStartPos").getValue();
        int currentExPtr = audioProcessor.lpcChannels[0].getCurrentExPtr();
        float startPosInSamples = exStart * audioProcessor.factoryExcitations[selectedExcitation][0].size();
        
        waveformViewer.setPlayheadPosition(startPosInSamples, static_cast<float>(currentExPtr));
    }
    else
    {
        waveformViewer.clearWaveform();
    }
}
