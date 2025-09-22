#pragma once

#include <JuceHeader.h>

const std::array<const char*, 11> excitationNames = {
    "WhiteNoise",
    "GreenNoise",
    "Shake",
    "StringScratch",
    "Off"
};

namespace ColorScheme
{
    const juce::Colour readingsColour = juce::Colour(0xffe03c31);
    const juce::Colour highlightColour = juce::Colour(0xff53565A);
    const juce::Colour bgColour = juce::Colour(0xffbbbcbc);
    const juce::Colour fillColour = juce::Colours::black;
}
