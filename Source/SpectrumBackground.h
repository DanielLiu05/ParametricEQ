/*
  ==============================================================================

    SpectrumBackground.h
    Created: 15 May 2026 12:36:49am
    Author:  Daniel

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpectrumBackground : public juce::Component, private juce::Timer
{
public:
    SpectrumBackground(ParametricEQAudioProcessor& p);
    ~SpectrumBackground() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    // Shared Y-axis mapping constants (Must match EQCurveComponent)
    constexpr static float MIN_DB = -60.0f;
    constexpr static float MAX_DB = 24.0f;

    float getXForFreq(float freq) const;
    float getYForDB(float db) const;

    ParametricEQAudioProcessor& processor;
    juce::Path spectrumPath;
};