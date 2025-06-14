//
//  sampler.cpp
//  LPMorph
//
//  Created by Anthony Hong on 2025-06-14.
//

#include "sampler.h"

WaveformViewer::WaveformViewer() : currentWaveform(nullptr), playheadStartPos(0.0f), playheadCurrentPos(0.0f), showPlayhead(false)
{
    setSize(400, 150);
}

WaveformViewer::~WaveformViewer()
{
}

void WaveformViewer::paint(juce::Graphics& g)
{
    g.fillAll(ColorScheme::bgColour);
    
    if (currentWaveform != nullptr && !currentWaveform->empty())
    {
        g.setColour(ColorScheme::fillColour);
        g.strokePath(waveformPath, juce::PathStrokeType(1.0f));
        
        if (showPlayhead)
        {
            g.setColour(ColorScheme::readingsColour);
            float playheadX = (playheadCurrentPos / static_cast<float>(currentWaveform->size())) * getWidth();
            g.drawVerticalLine(static_cast<int>(playheadX), 0.0f, static_cast<float>(getHeight()));
        }
    }
    else
    {
        g.setColour(juce::Colours::grey);
        g.drawRect(getLocalBounds(), 1);
        g.setColour(ColorScheme::readingsColour);
        g.drawText("No excitation loaded", getLocalBounds(), juce::Justification::centred);
    }
}

void WaveformViewer::resized()
{
    generateWaveformPath();
}

void WaveformViewer::setWaveform(const std::vector<double>* waveform)
{
    currentWaveform = waveform;
    generateWaveformPath();
    repaint();
}

void WaveformViewer::clearWaveform()
{
    currentWaveform = nullptr;
    waveformPath.clear();
    showPlayhead = false;
    repaint();
}

void WaveformViewer::setPlayheadPosition(float startPos, float currentPos)
{
    playheadStartPos = startPos;
    playheadCurrentPos = currentPos;
    showPlayhead = (currentWaveform != nullptr && !currentWaveform->empty());
    repaint();
}

void WaveformViewer::generateWaveformPath()
{
    waveformPath.clear();
    
    if (currentWaveform == nullptr || currentWaveform->empty() || getWidth() <= 0 || getHeight() <= 0)
        return;
    
    const auto& waveform = *currentWaveform;
    const int numSamples = static_cast<int>(waveform.size());
    const float width = static_cast<float>(getWidth());
    const float height = static_cast<float>(getHeight());
    const float centerY = height * 0.5f;
    const float amplitude = height * 0.4f;
    
    const int maxPointsToDisplay = juce::jmin(numSamples, static_cast<int>(width * 2));
    const int samplesPerPoint = juce::jmax(1, numSamples / maxPointsToDisplay);
    
    bool pathStarted = false;
    
    for (int i = 0; i < maxPointsToDisplay; ++i)
    {
        int sampleIndex = i * samplesPerPoint;
        if (sampleIndex >= numSamples) break;
        
        double maxVal = 0.0;
        double minVal = 0.0;
        
        for (int j = 0; j < samplesPerPoint && (sampleIndex + j) < numSamples; ++j)
        {
            double sample = waveform[sampleIndex + j];
            maxVal = juce::jmax(maxVal, sample);
            minVal = juce::jmin(minVal, sample);
        }
        
        float x = (static_cast<float>(i) / static_cast<float>(maxPointsToDisplay - 1)) * width;
        float yMax = centerY - static_cast<float>(maxVal) * amplitude;
        float yMin = centerY - static_cast<float>(minVal) * amplitude;
        
        yMax = juce::jlimit(0.0f, height, yMax);
        yMin = juce::jlimit(0.0f, height, yMin);
        
        if (!pathStarted)
        {
            waveformPath.startNewSubPath(x, yMax);
            pathStarted = true;
        }
        else
        {
            waveformPath.lineTo(x, yMax);
        }
        
        if (yMax != yMin)
        {
            waveformPath.lineTo(x, yMin);
        }
    }
}
