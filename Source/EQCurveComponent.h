/*
  ==============================================================================

    EQCurveComponent.h
    Created: 15 May 2026 12:36:21am
    Author:  Daniel

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h" 

class EQCurveComponent : public juce::Component,
    private juce::Timer
{
public:
    explicit EQCurveComponent(ParametricEQAudioProcessor& p);
    ~EQCurveComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // juce::Timer callback for smooth UI updates (60Hz)
    void timerCallback() override;

    // Updates the juce::Path based on the latest curve data from the processor
    void updateCurvePath();

    // Mouse interaction overrides
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Coordinate mapping helpers (Logarithmic Frequency <-> X, Linear Gain <-> Y)
    float getXForFreq(float freq) const;
    float getFreqForX(float x) const;
    float getYForGain(float gain) const;
    float getGainForY(float y) const;

    // Reference to the audio processor to fetch parameters and curve data
    ParametricEQAudioProcessor& processor;

    // Cached drawing data
    juce::Path curvePath;
    std::vector<juce::Point<float>> curvePoints;

    // Dragging state
    int draggedBandIndex = -1;
    float dragStartQ = 1.0f;

    // Constants for UI and mapping
    static constexpr int numBands = 5;
    static constexpr float handleRadius = 6.0f;
    static constexpr float MIN_DB = -24.0f;
    static constexpr float MAX_DB = 24.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQCurveComponent)
};