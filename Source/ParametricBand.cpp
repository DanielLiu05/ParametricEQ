/*
  ==============================================================================

    ParametricBand.cpp
    Created: 15 May 2026 12:34:48am
    Author:  Daniel

  ==============================================================================
*/

#include "ParametricBand.h"

namespace ParametricBand
{
    float computeMagnitudeAtFrequency(float targetFreq, float f, float gainDB, float q, ParamDefs::BandType type, double sampleRate)
    {
        // Instantiating a temporary filter on the Message Thread is perfectly safe 
        // and allows us to reuse the exact same math used in the audio thread.
        BiquadFilter tempFilter;
        juce::dsp::ProcessSpec spec{ sampleRate, 512, 1 };
        tempFilter.prepare(spec);
        tempFilter.updateCoefficients(f, gainDB, q, type, sampleRate);

        float mag = tempFilter.getMagnitudeAtFrequency(targetFreq);
        return juce::Decibels::gainToDecibels(mag);
    }
}