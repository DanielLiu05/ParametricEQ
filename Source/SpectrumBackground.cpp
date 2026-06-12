/*
  ==============================================================================

    SpectrumBackground.cpp
    Created: 15 May 2026 12:36:49am
    Author:  Daniel

  ==============================================================================
*/

#include "SpectrumBackground.h"

SpectrumBackground::SpectrumBackground(ParametricEQAudioProcessor& p) : processor(p)
{
    setOpaque(true);
    startTimerHz(45); // Update spectrum at 45 Hz
    setInterceptsMouseClicks(false, false); // Let mouse events pass through to the EQ Curve layer
}

SpectrumBackground::~SpectrumBackground()
{
    stopTimer();
}

void SpectrumBackground::timerCallback()
{
    std::vector<float> mags;
    processor.getSpectrumAnalyzer().getLatestSpectrum(mags);

    if (mags.empty()) return;

    spectrumPath.clear();
    float w = (float)getWidth();
    float h = (float)getHeight();
    if (w <= 0 || h <= 0) return;

    double sr = processor.getSampleRate();
    int fftSize = 2048; // Must match SpectrumAnalyzer setup
    float binToFreq = (float)sr / (float)fftSize;

    bool firstPoint = true;

    for (int i = 1; i < (int)mags.size(); ++i) // Skip DC bin (i=0)
    {
        float freq = i * binToFreq;
        if (freq < 20.0f || freq > 20000.0f) continue;

        float db = mags[i];
        if (db < MIN_DB) db = MIN_DB;
        if (db > MAX_DB) db = MAX_DB;

        float x = getXForFreq(freq);
        float y = getYForDB(db);

        if (firstPoint)
        {
            spectrumPath.startNewSubPath(x, h); // Start from bottom left
            spectrumPath.lineTo(x, y);
            firstPoint = false;
        }
        else
        {
            spectrumPath.lineTo(x, y);
        }
    }

    if (!firstPoint)
    {
        spectrumPath.lineTo(getXForFreq(20000.0f), h); // Bottom right
        spectrumPath.closeSubPath();
    }

    repaint();
}

void SpectrumBackground::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(20, 24, 32)); // Dark background

    if (spectrumPath.isEmpty()) return;

    // Create a vertical gradient for the spectrum fill
    juce::ColourGradient gradient(
        juce::Colours::cyan.withAlpha(0.5f), 0.0f, 0.0f,
        juce::Colours::blue.withAlpha(0.05f), 0.0f, (float)getHeight(),
        false
    );
    g.setGradientFill(gradient);
    g.fillPath(spectrumPath);

    // Draw a crisp line on top of the fill
    g.setColour(juce::Colours::cyan.withAlpha(0.8f));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
}

void SpectrumBackground::resized() {}

float SpectrumBackground::getXForFreq(float freq) const
{
    float w = (float)getWidth();
    if (freq <= 20.0f) return 0.0f;
    if (freq >= 20000.0f) return w;
    float logMin = std::log10(20.0f);
    float logMax = std::log10(20000.0f);
    return w * (std::log10(freq) - logMin) / (logMax - logMin);
}

float SpectrumBackground::getYForDB(float db) const
{
    float h = (float)getHeight();
    float norm = (db - MIN_DB) / (MAX_DB - MIN_DB);
    return h * (1.0f - norm); // Y is inverted in UI coordinates
}
