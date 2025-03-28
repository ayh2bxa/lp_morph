/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VoicemorphAudioProcessor::VoicemorphAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), 
lpc(2), apvts(*this, nullptr, "Parameters", Utility::ParameterHelper::createParameterLayout())
#endif
{
    loadFactoryExcitations();
}

VoicemorphAudioProcessor::~VoicemorphAudioProcessor()
{
}

// Function to load a WAV file into a vector<float>
std::vector<double> loadWavToBuffer(const juce::File& file)
{
    // Check if the file exists
    if (!file.existsAsFile())
    {
        DBG("File not found: " + file.getFullPathName());
        return {};
    }
    // Step 1: Register the available audio formats (WAV, AIFF, etc.)
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    // Step 2: Create an AudioFormatReader for the file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
    {
        // Failed to open file; return an empty vector
        return {};
    }

    // Step 3: Create an AudioBuffer to hold the samples.
    // reader->lengthInSamples gives the total number of samples per channel.
    const int numChannels = static_cast<int>(reader->numChannels);
    const int numSamples = static_cast<int>(reader->lengthInSamples);
    juce::AudioBuffer<float> buffer(numChannels, numSamples);

    // Read the samples into the buffer.
    reader->read(&buffer,           // destination buffer
                 0,                 // start writing at sample 0 in the buffer
                 numSamples,        // number of samples to read
                 0,                 // starting sample in the source file
                 true,              // use left channel if numChannels < expected?
                 true);             // use right channel

    // Step 4: Convert AudioBuffer data to a single vector<float>.
    // In this example, we average the samples across all channels.
    std::vector<double> audioData(numSamples, 0.0f);
    for (int sample = 0; sample < numSamples; ++sample)
    {
        double mixedSample = 0.0f;
        for (int channel = 0; channel < numChannels; ++channel)
        {
            mixedSample += static_cast<double>(buffer.getReadPointer(channel)[sample]);
        }
        audioData[sample] = mixedSample / numChannels;
    }
    return audioData;
}

void VoicemorphAudioProcessor::loadFactoryExcitations() {
    juce::File bassyTrainFile{"/Users/anthony/Desktop/Careers/portfolio/linear_predictive_coding/resources/excitations/BassyTrainNoise.wav"};
    factoryExcitations.push_back(loadWavToBuffer(bassyTrainFile));
    
    juce::File cherubScreamsFile{"/Users/anthony/Desktop/Careers/portfolio/linear_predictive_coding/resources/excitations/CherubScreams.wav"};
    factoryExcitations.push_back(loadWavToBuffer(cherubScreamsFile));
    
    juce::File micScratchFile{"/Users/anthony/Desktop/Careers/portfolio/linear_predictive_coding/resources/excitations/MicScratch.wav"};
    factoryExcitations.push_back(loadWavToBuffer(micScratchFile));
    
    juce::File ringFile{"/Users/anthony/Desktop/Careers/portfolio/linear_predictive_coding/resources/excitations/Ring.wav"};
    factoryExcitations.push_back(loadWavToBuffer(ringFile));
    
    juce::File trainScreech1File{"/Users/anthony/Desktop/Careers/portfolio/linear_predictive_coding/resources/excitations/TrainScreech1.wav"};
    factoryExcitations.push_back(loadWavToBuffer(trainScreech1File));
    
    juce::File trainScreech2File{"/Users/anthony/Desktop/Careers/portfolio/linear_predictive_coding/resources/excitations/TrainScreech2.wav"};
    factoryExcitations.push_back(loadWavToBuffer(trainScreech2File));
    
    juce::File whiteNoiseFile{"/Users/anthony/Desktop/Careers/portfolio/linear_predictive_coding/resources/excitations/WhiteNoise.wav"};
    factoryExcitations.push_back(loadWavToBuffer(whiteNoiseFile));
    lpc.noise = &factoryExcitations[6];
}

//==============================================================================
const juce::String VoicemorphAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VoicemorphAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VoicemorphAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VoicemorphAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VoicemorphAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VoicemorphAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VoicemorphAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VoicemorphAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String VoicemorphAudioProcessor::getProgramName (int index)
{
    return {};
}

void VoicemorphAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void VoicemorphAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    previousGain = apvts.getParameterAsValue("input gain").getValue();
    previousGain = juce::Decibels::decibelsToGain(previousGain);
}

void VoicemorphAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VoicemorphAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void VoicemorphAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    float lpcMix = apvts.getParameterAsValue("lpc mix").getValue();
    float percentage = apvts.getParameterAsValue("ex len").getValue();
    float exStartPos = apvts.getParameterAsValue("ex start pos").getValue();
    int prevExType = lpc.exType;
    lpc.exType = apvts.getParameterAsValue("ex type").getValue();
    lpc.noise = &factoryExcitations[lpc.exType];
    lpc.EXLEN = (*lpc.noise).size();
    int prevOrder = lpc.ORDER;
    lpc.ORDER = apvts.getParameterAsValue("lpc order").getValue();
    lpc.orderChanged = prevOrder != lpc.ORDER;
    lpc.exTypeChanged = prevExType != lpc.exType;
    currentGain = apvts.getParameterAsValue("input gain").getValue();
    currentGain = juce::Decibels::decibelsToGain(currentGain);
    for (int ch = 0; ch < totalNumOutputChannels; ch++) {
        auto *channelData = buffer.getWritePointer(ch);
        if (juce::approximatelyEqual(currentGain, previousGain)) {
            buffer.applyGain(currentGain);
        }
        else {
            buffer.applyGainRamp(0, buffer.getNumSamples(), previousGain, currentGain);
            previousGain = currentGain;
        }
        lpc.applyLPC(channelData, buffer.getNumSamples(), lpcMix, percentage, ch, exStartPos);
    }
}

//==============================================================================
bool VoicemorphAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VoicemorphAudioProcessor::createEditor()
{
    return new VoicemorphAudioProcessorEditor (*this, apvts);
}

//==============================================================================
void VoicemorphAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void VoicemorphAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoicemorphAudioProcessor();
}
