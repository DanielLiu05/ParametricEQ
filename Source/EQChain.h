/*
  ==============================================================================

    EQChain.h
    Created: 15 May 2026 12:35:35am
    Author:  Daniel

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "ParameterDefinitions.h"
#include "ParametricBand.h"

/**
 * Wraps 5 Biquad filters in series to form a 5-band Parametric EQ.
 */
class EQChain
{
public:
    EQChain() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void releaseResources();

    template <typename ProcessContext>
    void process(const ProcessContext& context) noexcept;

    void setBandBypass(int bandIndex, bool shouldBypass);
    void updateBandCoefficients(int bandIndex, float freq, float gain, float q, ParamDefs::BandType type, double sampleRate);

private:
    std::array<ParametricBand::BiquadFilter, numBands> filters;
    std::array<bool, numBands> bypassed{ false, false, false, false, false };
};