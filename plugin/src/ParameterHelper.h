#pragma once

#include <JuceHeader.h>

#define MAX_ORDER 100
#define MAX_FRAME_DUR 50

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
                std::make_unique<AudioParameterFloat>(juce::ParameterID("wetGain", 1), "Wet Gain", NormalisableRange<float>{-40.f, 20.f, 0.1f}, 0.f),
                std::make_unique<AudioParameterFloat>(juce::ParameterID("lpcMix", 1), "LPC Mix", NormalisableRange<float>{0.f, 1.f, 0.01f}, 1.f),
                std::make_unique<AudioParameterFloat>(juce::ParameterID("exLen", 1), "Excitation Length", NormalisableRange<float>{0.0001f, 1.f, 0.0001f, 0.3f}, 1.f),
                std::make_unique<AudioParameterFloat>(juce::ParameterID("exStartPos", 1), "Excitation Start Position", NormalisableRange<float>{0.0f, 1.f, 0.01f}, 0.f),
                std::make_unique<AudioParameterInt>(juce::ParameterID("exType", 1), "Excitation Type", 0, 7, 6),
                std::make_unique<AudioParameterInt>(juce::ParameterID("lpcOrder", 1), "LPC Order", 1, MAX_ORDER, 100),
                std::make_unique<AudioParameterFloat>(juce::ParameterID("frameDur", 1), "Frame Duration (ms)", NormalisableRange<float>{0.1f, (float)MAX_FRAME_DUR, 0.01f}, 50.f),
                std::make_unique<AudioParameterBool>(juce::ParameterID("useSidechain", 1), "Use Sidechain as Excitation", false)
            };
        }
    };
}
