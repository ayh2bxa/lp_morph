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
                     #if ! JucePlugin_IsSynth
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     #endif
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                    #endif
                      ),
lpc(2), apvts(*this, nullptr, juce::Identifier ("Parameters"), Utility::ParameterHelper::createParameterLayout())
#endif
{
    loadFactoryExcitations();
    exLenParameter = apvts.getRawParameterValue ("ex len");
    gainParameter = apvts.getRawParameterValue ("wet gain");
    lpcMixParameter = apvts.getRawParameterValue ("lpc mix");
    lpcExStartParameter = apvts.getRawParameterValue ("ex start pos");
    lpcOrderParameter = apvts.getRawParameterValue ("lpc order");
    lpcExTypeParameter = apvts.getRawParameterValue ("ex type");
    frameDurParameter = apvts.getRawParameterValue ("frame dur");
    isStandalone = wrapperType == wrapperType_Standalone;
}

VoicemorphAudioProcessor::~VoicemorphAudioProcessor()
{
}

// Function to load a WAV file into a vector<float>
std::vector<double> loadEmbeddedWavToBuffer(const void* data, size_t dataSize)
{
    if (data != nullptr && dataSize > 0) {
        size_t numSamples = dataSize / 2;
        const int16_t* sampleData = static_cast<const int16_t*>(data);
        std::vector<double> samples;
        samples.reserve(numSamples);
        for (size_t i = 0; i < numSamples; ++i)
        {
            samples.push_back(static_cast<double>(sampleData[i]) / 32768.0);
        }
        return samples;
    }
    return {};
}

void VoicemorphAudioProcessor::loadFactoryExcitations() {
    auto bassyTrainAudio = loadEmbeddedWavToBuffer(BinaryData::BassyTrainNoise_wav_bin, BinaryData::BassyTrainNoise_wav_binSize);
    factoryExcitations.push_back(bassyTrainAudio);
    
    auto cherubScreamsAudio = loadEmbeddedWavToBuffer(BinaryData::CherubScreams_wav_bin, BinaryData::CherubScreams_wav_binSize);
    factoryExcitations.push_back(cherubScreamsAudio);
    
    auto micScratchAudio = loadEmbeddedWavToBuffer(BinaryData::MicScratch_wav_bin, BinaryData::MicScratch_wav_binSize);
    factoryExcitations.push_back(micScratchAudio);
    
    auto ringAudio = loadEmbeddedWavToBuffer(BinaryData::Ring_wav_bin, BinaryData::Ring_wav_binSize);
    factoryExcitations.push_back(ringAudio);
    
    auto trainScreech1Audio = loadEmbeddedWavToBuffer(BinaryData::TrainScreech1_wav_bin, BinaryData::TrainScreech1_wav_binSize);
    factoryExcitations.push_back(trainScreech1Audio);
    
    auto trainScreech2Audio = loadEmbeddedWavToBuffer(BinaryData::TrainScreech2_wav_bin, BinaryData::TrainScreech2_wav_binSize);
    factoryExcitations.push_back(trainScreech2Audio);
    
    auto whiteNoiseAudio = loadEmbeddedWavToBuffer(BinaryData::WhiteNoise_wav_bin, BinaryData::WhiteNoise_wav_binSize);
    factoryExcitations.push_back(whiteNoiseAudio);

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

void VoicemorphAudioProcessor::updateLpcParams() {
    float lpcMix = (*lpcMixParameter).load();
    float exStartPos = (*lpcExStartParameter).load();
    int prevExType = lpc.exType;
    lpc.exType = static_cast<int>((*lpcExTypeParameter).load());
    
    if (usingCustomExcitation && currentCustomExcitationIndex >= 0 && currentCustomExcitationIndex < customExcitations.size()) {
        lpc.noise = &customExcitations[currentCustomExcitationIndex];
        lpc.EXLEN = (*lpc.noise).size();
    } else if (lpc.exType == 7) {
        lpc.noise = nullptr;
        lpc.EXLEN = 0;
    } else if (lpc.exType >= 0 && lpc.exType < factoryExcitations.size()) {
        lpc.noise = &factoryExcitations[lpc.exType];
        lpc.EXLEN = (*lpc.noise).size();
    } else {
        lpc.noise = nullptr;
        lpc.EXLEN = 0;
    }
    
    int prevOrder = lpc.ORDER;
    lpc.ORDER = static_cast<int>((*lpcOrderParameter).load());
    lpc.orderChanged = prevOrder != lpc.ORDER;
    lpc.exTypeChanged = prevExType != lpc.exType;
    lpc.exStartChanged = lpc.exStart != exStartPos;
    lpc.prevFrameLen = lpc.FRAMELEN;
    lpc.FRAMELEN = static_cast<int>((*frameDurParameter).load()*lpc.SAMPLERATE/1000.0);
    if (lpc.prevFrameLen != lpc.FRAMELEN) {
        for (int i = 0; i < lpc.FRAMELEN; i++) {
            lpc.window[i] = 0.5*(1.0-cos(2.0*M_PI*i/(double)(lpc.FRAMELEN-1)));
        }
        lpc.HOPSIZE = lpc.FRAMELEN/2;
    }
    lpc.gainDb = 0.0;
}

//==============================================================================
void VoicemorphAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    previousGain = (*gainParameter).load();
    previousGain = juce::Decibels::decibelsToGain(previousGain);
    updateLpcParams();
    lpc.prepareToPlay();
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
    int numChannels = totalNumOutputChannels;
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    updateLpcParams();
    currentGain = (*gainParameter).load();
    currentGain = juce::Decibels::decibelsToGain(currentGain);
    for (int ch = 0; ch < numChannels; ch++) {
        auto *channelDataR = buffer.getReadPointer(ch);
        auto *channelDataW = buffer.getWritePointer(ch);
        lpc.applyLPC(channelDataR, channelDataW, buffer.getNumSamples(), (*lpcMixParameter).load(), (*exLenParameter).load(), ch, (*lpcExStartParameter).load(), nullptr, previousGain, currentGain);
//        float rms = buffer.getRMSLevel(ch, 0, buffer.getNumSamples());
//        if (rms > 1) {
//            buffer.applyGain(ch, 0, buffer.getNumSamples(), 1.f/rms);
//        }
//        else if (isnan(rms)) {
//            buffer.applyGain(ch, 0, buffer.getNumSamples(), 0.f);
//        }
    }
    if (!juce::approximatelyEqual(currentGain, previousGain)) {
        previousGain = currentGain;
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
std::vector<double> loadWavFile(const juce::File& file)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader != nullptr)
    {
        const int numSamples = static_cast<int>(reader->lengthInSamples);
        juce::AudioBuffer<float> buffer(1, numSamples);
        reader->read(&buffer, 0, numSamples, 0, true, false);
        
        std::vector<double> samples;
        samples.reserve(numSamples);
        const float* channelData = buffer.getReadPointer(0);
        for (int i = 0; i < numSamples; ++i)
        {
            samples.push_back(static_cast<double>(channelData[i]));
        }
        return samples;
    }
    return {};
}

void VoicemorphAudioProcessor::loadCustomExcitations(const juce::File& selectedFile)
{
    customExcitations.clear();
    customExcitationFiles.clear();
    
    juce::File parentDir = selectedFile.getParentDirectory();
    juce::Array<juce::File> wavFiles;
    parentDir.findChildFiles(wavFiles, juce::File::findFiles, false, "*.wav");
    
    for (const auto& file : wavFiles)
    {
        auto samples = loadWavFile(file);
        if (!samples.empty())
        {
            customExcitations.push_back(samples);
            customExcitationFiles.push_back(file);
        }
    }
}

void VoicemorphAudioProcessor::setCustomExcitation(int index)
{
    if (index >= 0 && index < customExcitations.size())
    {
        currentCustomExcitationIndex = index;
    }
}

std::vector<juce::File> VoicemorphAudioProcessor::getCustomExcitationFiles() const
{
    return customExcitationFiles;
}

void VoicemorphAudioProcessor::setUsingCustomExcitation(bool useCustom)
{
    usingCustomExcitation = useCustom;
}

bool VoicemorphAudioProcessor::isUsingCustomExcitation() const
{
    return usingCustomExcitation;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoicemorphAudioProcessor();
}
