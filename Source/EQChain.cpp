/*
  ==============================================================================

    EQChain.cpp
    Created: 15 May 2026 12:35:35am
    Author:  Daniel

  ==============================================================================
*/

#include "EQChain.h"

// Type traits to detect if a ProcessContext has getInputBlock() and getOutputBlock() methods
template<typename T, typename = void>
struct hasGetInputBlock : std::false_type {};

template<typename T>
struct hasGetInputBlock<T, std::void_t<decltype(std::declval<T>().getInputBlock())>> : std::true_type {};

template<typename T, typename = void>
struct hasGetOutputBlock : std::false_type {};

template<typename T>
struct hasGetOutputBlock<T, std::void_t<decltype(std::declval<T>().getOutputBlock())>> : std::true_type {};

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
            // Use SFINAE to detect if we're in a non-replacing context
            if constexpr (hasGetInputBlock<ProcessContext>::value && hasGetOutputBlock<ProcessContext>::value)
            {
                auto&& inputBlock = context.getInputBlock();
                auto&& outputBlock = context.getOutputBlock();
                // Compare channel counts and sample counts to determine if blocks are the same
                if (inputBlock.getNumChannels() != outputBlock.getNumChannels() ||
                    inputBlock.getNumSamples() != outputBlock.getNumSamples())
                {
                    outputBlock.copyFrom(inputBlock);
                }
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