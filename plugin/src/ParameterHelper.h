#pragma once

#include <JuceHeader.h>

using namespace juce;

namespace Utility
{
    class ParameterHelper
    {
    public:
        ParameterHelper() = delete;

        static AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
        {
            return AudioProcessorValueTreeState::ParameterLayout{
                std::make_unique<AudioParameterFloat>(juce::ParameterID("wet gain", 1), "Wet Gain", NormalisableRange<float>{-40.f, 6.f, 0.1f}, 0.f),
                std::make_unique<AudioParameterFloat>(juce::ParameterID("lpc mix", 1), "LPC Mix", NormalisableRange<float>{0.f, 1.f, 0.01f}, 0.f),
                std::make_unique<AudioParameterFloat>(juce::ParameterID("ex len", 1), "Excitation Length", NormalisableRange<float>{0.001f, 1.f, 0.001f, 0.2f}, 1.f),
                std::make_unique<AudioParameterFloat>(juce::ParameterID("ex start pos", 1), "Excitation Start Position", NormalisableRange<float>{0.0f, 1.f, 0.01f}, 0.f),
                std::make_unique<AudioParameterInt>(juce::ParameterID("ex type", 1), "Excitation Type", 0, 6, 6),
                std::make_unique<AudioParameterInt>(juce::ParameterID("lpc order", 1), "LPC Order", 1, 32, 32),
                std::make_unique<AudioParameterFloat>(juce::ParameterID("frame dur", 1), "Frame Duration (ms)", NormalisableRange<float>{1.f, 10.f, 0.1f}, 10.f),
                std::make_unique<AudioParameterBool>(juce::ParameterID("use sidechain", 1), "Use Sidechain as Excitation", false)
            };
        }
    };
}
