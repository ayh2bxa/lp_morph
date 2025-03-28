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
                std::make_unique<AudioParameterFloat>("input gain", "Input Gain", NormalisableRange<float>{-40.f, 6.f, 0.1f}, 0.f),
//                std::make_unique<AudioParameterFloat>("output gain", "Input Gain", NormalisableRange<float>{-40.f, 0.f, 0.1f}, -6.f),
                std::make_unique<AudioParameterFloat>("lpc mix", "LPC Mix", NormalisableRange<float>{0.f, 1.f, 0.01f}, 0.f),
                std::make_unique<AudioParameterFloat>("ex len", "Excitation Length", NormalisableRange<float>{0.0005f, 1.f, 0.00001f, 0.1f}, 1.f),
                std::make_unique<AudioParameterFloat>("ex start pos", "Excitation Start Position", NormalisableRange<float>{0.0f, 1.f, 0.01f}, 0.f),
                std::make_unique<AudioParameterInt>("ex type", "Excitation Type", 0, 6, 6),
                std::make_unique<AudioParameterInt>("lpc order", "LPC Order", 1, 32, 32)
            };
        }
    };
}
