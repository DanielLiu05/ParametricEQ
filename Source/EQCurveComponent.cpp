/*
  ==============================================================================

    EQCurveCoomponent.cpp
    Created: 15 May 2026 12:36:21am
    Author:  Daniel

  ==============================================================================
*/

#include "EQCurveComponent.h"

EQCurveComponent::EQCurveComponent(ParametricEQAudioProcessor& p) : processor(p)
{
    setOpaque(false); // CRITICAL: Allows the SpectrumBackground to show through
    startTimerHz(60); // 60 FPS for buttery smooth dragging
}

EQCurveComponent::~EQCurveComponent()
{
    stopTimer();
}

void EQCurveComponent::timerCallback()
{
    updateCurvePath();
    repaint();
}

void EQCurveComponent::updateCurvePath()
{
    curvePath.clear();
    curvePoints.clear();

    // Fetch cached curve (Instantaneous because it reads from Atomics)
    if (!processor.getEQCurveData(curvePoints) || curvePoints.empty())
        return;

    float w = (float)getWidth();
    float h = (float)getHeight();
    if (w <= 0 || h <= 0) return;

    bool first = true;
    for (const auto& pt : curvePoints)
    {
        float x = getXForFreq(pt.x);
        float y = getYForGain(pt.y);

        if (first) { curvePath.startNewSubPath(x, y); first = false; }
        else { curvePath.lineTo(x, y); }
    }
}

void EQCurveComponent::paint(juce::Graphics& g)
{
    float w = (float)getWidth();
    float h = (float)getHeight();

    // 1. Draw 0dB Grid Line
    float y0 = getYForGain(0.0f);
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawHorizontalLine((int)y0, 0.0f, w);

    // 2. Draw EQ Curve Fill & Stroke
    if (!curvePath.isEmpty())
    {
        juce::Path filledPath(curvePath);
        filledPath.lineTo(w, h);
        filledPath.lineTo(0.0f, h);
        filledPath.closeSubPath();

        g.setColour(juce::Colours::orange.withAlpha(0.2f));
        g.fillPath(filledPath);

        g.setColour(juce::Colours::orange);
        g.strokePath(curvePath, juce::PathStrokeType(2.5f));
    }

    // 3. Draw Interactive Band Handles
    for (int i = 0; i < numBands; ++i)
    {
        if (processor.getTargetBypass(i) > 0.5f) continue;

        float f = processor.getTargetFreq(i);
        float gain = processor.getTargetGain(i);

        float x = getXForFreq(f);
        float y = getYForGain(gain);

        juce::Colour handleColor = (i == draggedBandIndex) ? juce::Colours::yellow : juce::Colours::white;

        g.setColour(handleColor);
        g.fillEllipse(x - handleRadius, y - handleRadius, handleRadius * 2.0f, handleRadius * 2.0f);
        g.setColour(juce::Colours::black);
        g.drawEllipse(x - handleRadius, y - handleRadius, handleRadius * 2.0f, handleRadius * 2.0f, 1.5f);

        g.drawText(juce::String(i + 1),
            juce::Rectangle<int>((int)x - 10, (int)y - 10, 20, 20),
            juce::Justification::centred);
    }
}

void EQCurveComponent::resized() {}

void EQCurveComponent::mouseDown(const juce::MouseEvent& e)
{
    draggedBandIndex = -1;

    // Hit-test to find if user clicked on a handle
    float minDist = 1e9f;
    for (int i = 0; i < numBands; ++i)
    {
        if (processor.getTargetBypass(i) > 0.5f) continue;

        float x = getXForFreq(processor.getTargetFreq(i));
        float y = getYForGain(processor.getTargetGain(i));

        float dist = std::hypot(e.x - x, e.y - y);
        if (dist < handleRadius * 2.5f && dist < minDist)
        {
            minDist = dist;
            draggedBandIndex = i;
            dragStartQ = processor.getTargetQ(i); // Store for Shift-Drag
        }
    }
}

void EQCurveComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggedBandIndex < 0 || draggedBandIndex >= numBands) return;

    juce::String prefix = "band" + juce::String(draggedBandIndex) + "_";

    if (e.mods.isShiftDown())
    {
        // Shift + Drag: Adjust Q (Vertical drag maps to Q)
        float deltaQ = e.getDistanceFromDragStartY() * -0.05f;
        float newQ = juce::jlimit(0.1f, 10.0f, dragStartQ + deltaQ);
        processor.getAPVTS().getParameterAsValue(prefix + "q").setValue(newQ);
    }
    else
    {
        // Normal Drag: Adjust Freq (X) and Gain (Y)
        float newFreq = getFreqForX((float)e.x);
        float newGain = getGainForY((float)e.y);

        newFreq = juce::jlimit(20.0f, 20000.0f, newFreq);
        newGain = juce::jlimit(-24.0f, 24.0f, newGain);

        // Push to APVTS (Triggers thread-safe parameterChanged on Message Thread)
        processor.getAPVTS().getParameterAsValue(prefix + "freq").setValue(newFreq);
        processor.getAPVTS().getParameterAsValue(prefix + "gain").setValue(newGain);
    }
}

void EQCurveComponent::mouseUp(const juce::MouseEvent&)
{
    draggedBandIndex = -1;
}

// --- Coordinate Mapping ---
float EQCurveComponent::getXForFreq(float freq) const
{
    float w = (float)getWidth();
    if (freq <= 20.0f) return 0.0f;
    if (freq >= 20000.0f) return w;
    float logMin = std::log10(20.0f);
    float logMax = std::log10(20000.0f);
    return w * (std::log10(freq) - logMin) / (logMax - logMin);
}

float EQCurveComponent::getFreqForX(float x) const
{
    float w = (float)getWidth();
    if (w <= 0) return 20.0f;
    float logMin = std::log10(20.0f);
    float logMax = std::log10(20000.0f);
    return std::pow(10.0f, logMin + (x / w) * (logMax - logMin));
}

float EQCurveComponent::getYForGain(float gain) const
{
    float h = (float)getHeight();
    float norm = (gain - MIN_DB) / (MAX_DB - MIN_DB);
    return h * (1.0f - norm);
}

float EQCurveComponent::getGainForY(float y) const
{
    float h = (float)getHeight();
    if (h <= 0) return 0.0f;
    float norm = 1.0f - (y / h);
    return MIN_DB + norm * (MAX_DB - MIN_DB);
}
