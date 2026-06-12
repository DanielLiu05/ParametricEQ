/*
  ==============================================================================

    EQChain.cpp
    Created: 15 May 2026 12:35:35am
    Author:  Daniel

  ==============================================================================
*/

#include "EQChain.h"

void EQChain::prepare(const juce::dsp::ProcessSpec& spec)
{
    for (auto& filter : filters)
    {
        filter.prepare(spec);
        // Initialize with flat pass-through (Bell, 1kHz, 0dB, Q=1)
        filter.updateCoefficients(1000.0f, 0.0f, 1.0f, ParamDefs::BandType::Bell, spec.sampleRate);
    }
    std::fill(bypassed.begin(), bypassed.end(), false);
}

void EQChain::releaseResources()
{
    for (auto& filter : filters)
        filter.releaseResources();
}

template <typename ProcessContext>
void EQChain::process(const ProcessContext& context) noexcept
{
    for (int i = 0; i < numBands; ++i)
    {
        if (!bypassed[i])
        {
            filters[i].process(context);
        }
        else
        {
            // If bypassed and context is Non-Replacing, we must manually copy input to output
            if constexpr (!ProcessContext::isReplacing())
            {
                auto&& inputBlock = context.getInputBlock();
                auto&& outputBlock = context.getOutputBlock();
                outputBlock.copyFrom(inputBlock);
            }
        }
    }
}

// Explicit template instantiation to prevent linker errors
template void EQChain::process<juce::dsp::ProcessContextReplacing<float>>(const juce::dsp::ProcessContextReplacing<float>&) noexcept;
template void EQChain::process<juce::dsp::ProcessContextNonReplacing<float>>(const juce::dsp::ProcessContextNonReplacing<float>&) noexcept;

void EQChain::setBandBypass(int bandIndex, bool shouldBypass)
{
    if (bandIndex >= 0 && bandIndex < numBands)
        bypassed[bandIndex] = shouldBypass;
}

void EQChain::updateBandCoefficients(int bandIndex, float freq, float gain, float q, ParamDefs::BandType type, double sampleRate)
{
    if (bandIndex >= 0 && bandIndex < numBands)
    {
        filters[bandIndex].updateCoefficients(freq, gain, q, type, sampleRate);
    }
}