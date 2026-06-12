/*
  ==============================================================================

    ParametricBand.h
    Created: 15 May 2026 12:34:48am
    Author:  Daniel

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "ParameterDefinitions.h"
#include <cmath>
#include <vector>

namespace ParametricBand
{
    /**
     * A strict, real-time safe Biquad Filter using Transposed Direct Form II.
     * Avoids JUCE's heap allocations during coefficient updates.
     */
    class BiquadFilter
    {
    public:
        BiquadFilter() = default;

        void prepare(const juce::dsp::ProcessSpec& spec)
        {
            currentSampleRate = spec.sampleRate;
            filterState.resize(spec.numChannels);
            reset();
        }

        void reset()
        {
            for (auto& state : filterState)
                state.s1 = state.s2 = 0.0f;
        }

        void releaseResources()
        {
            filterState.clear();
            filterState.shrink_to_fit();
        }

        /**
         * Updates filter coefficients using RBJ Cookbook formulas.
         * 100% Real-Time Safe (No memory allocations, just math).
         */
        void updateCoefficients(float freq, float gainDB, float q, ParamDefs::BandType type, double sampleRate) noexcept
        {
            currentSampleRate = sampleRate;
            freq = juce::jlimit(20.0f, 20000.0f, freq);
            q = juce::jlimit(0.1f, 10.0f, q);

            float A = std::pow(10.0f, gainDB / 40.0f);
            float w0 = juce::MathConstants<float>::twoPi * freq / (float)sampleRate;
            float cosw0 = std::cos(w0);
            float sinw0 = std::sin(w0);
            float alpha = sinw0 / (2.0f * q);

            float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;

            switch (type)
            {
            case ParamDefs::BandType::Bell:
                b0 = 1.0f + alpha * A;
                b1 = -2.0f * cosw0;
                b2 = 1.0f - alpha * A;
                a0 = 1.0f + alpha / A;
                a1 = -2.0f * cosw0;
                a2 = 1.0f - alpha / A;
                break;
            case ParamDefs::BandType::LowShelf:
            {
                float alphaA = sinw0 / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / q - 1.0f) + 2.0f);
                b0 = A * ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alphaA);
                b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0);
                b2 = A * ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alphaA);
                a0 = (A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alphaA;
                a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0);
                a2 = (A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alphaA;
                break;
            }
            case ParamDefs::BandType::HighShelf:
            {
                float alphaA = sinw0 / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / q - 1.0f) + 2.0f);
                b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alphaA);
                b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0);
                b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alphaA);
                a0 = (A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alphaA;
                a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0);
                a2 = (A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alphaA;
                break;
            }
            case ParamDefs::BandType::LowPass:
                b0 = (1.0f - cosw0) / 2.0f; b1 = 1.0f - cosw0; b2 = (1.0f - cosw0) / 2.0f;
                a0 = 1.0f + alpha; a1 = -2.0f * cosw0; a2 = 1.0f - alpha;
                break;
            case ParamDefs::BandType::HighPass:
                b0 = (1.0f + cosw0) / 2.0f; b1 = -(1.0f + cosw0); b2 = (1.0f + cosw0) / 2.0f;
                a0 = 1.0f + alpha; a1 = -2.0f * cosw0; a2 = 1.0f - alpha;
                break;
            case ParamDefs::BandType::Notch:
                b0 = 1.0f; b1 = -2.0f * cosw0; b2 = 1.0f;
                a0 = 1.0f + alpha; a1 = -2.0f * cosw0; a2 = 1.0f - alpha;
                break;
            case ParamDefs::BandType::BandPass:
                b0 = alpha; b1 = 0.0f; b2 = -alpha;
                a0 = 1.0f + alpha; a1 = -2.0f * cosw0; a2 = 1.0f - alpha;
                break;
            default: // Pass-through
                b0 = 1.0f; b1 = 0.0f; b2 = 0.0f; a0 = 1.0f; a1 = 0.0f; a2 = 0.0f;
                break;
            }

            // Normalize by a0
            float invA0 = 1.0f / a0;
            coeffs.b0 = b0 * invA0;
            coeffs.b1 = b1 * invA0;
            coeffs.b2 = b2 * invA0;
            coeffs.a1 = a1 * invA0;
            coeffs.a2 = a2 * invA0;
        }

        template <typename ProcessContext>
        void process(const ProcessContext& context) noexcept
        {
            auto&& inputBlock = context.getInputBlock();
            auto&& outputBlock = context.getOutputBlock();
            auto numChannels = inputBlock.getNumChannels();
            auto numSamples = inputBlock.getNumSamples();

            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                auto* in = inputBlock.getChannelPointer(ch);
                auto* out = outputBlock.getChannelPointer(ch);

                // Ensure we don't overflow state array if spec changed unexpectedly
                if (ch >= filterState.size()) break;
                auto& state = filterState[ch];

                // Transposed Direct Form II (Optimal for floating point precision)
                for (size_t i = 0; i < numSamples; ++i)
                {
                    float x = in[i];
                    float y = coeffs.b0 * x + state.s1;
                    state.s1 = coeffs.b1 * x - coeffs.a1 * y + state.s2;
                    state.s2 = coeffs.b2 * x - coeffs.a2 * y;
                    out[i] = y;
                }
            }
        }

        /**
         * Computes the exact magnitude response at a specific frequency using the Z-transform.
         * Used by the UI thread to draw the EQ curve perfectly matching the DSP.
         */
        float getMagnitudeAtFrequency(float targetFreq) const noexcept
        {
            if (currentSampleRate <= 0.0) return 1.0f;
            float w = juce::MathConstants<float>::twoPi * targetFreq / (float)currentSampleRate;

            float cos1 = std::cos(w);
            float cos2 = std::cos(2.0f * w);
            float sin1 = std::sin(w);
            float sin2 = std::sin(2.0f * w);

            // Numerator: H(z) = b0 + b1*z^-1 + b2*z^-2
            float numReal = coeffs.b0 + coeffs.b1 * cos1 + coeffs.b2 * cos2;
            float numImag = -(coeffs.b1 * sin1 + coeffs.b2 * sin2);
            float numMagSq = numReal * numReal + numImag * numImag;

            // Denominator: 1 + a1*z^-1 + a2*z^-2 (a0 is normalized to 1)
            float denReal = 1.0f + coeffs.a1 * cos1 + coeffs.a2 * cos2;
            float denImag = -(coeffs.a1 * sin1 + coeffs.a2 * sin2);
            float denMagSq = denReal * denReal + denImag * denImag;

            if (denMagSq == 0.0f) return 0.0f;
            return std::sqrt(numMagSq / denMagSq);
        }

    private:
        struct FilterState { float s1 = 0.0f, s2 = 0.0f; };
        std::vector<FilterState> filterState;

        struct Coefficients { float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f; };
        Coefficients coeffs;

        double currentSampleRate = 44100.0;
    };

    /**
     * Helper for the UI thread to compute magnitude without maintaining a persistent filter state.
     */
    float computeMagnitudeAtFrequency(float targetFreq, float f, float gainDB, float q, ParamDefs::BandType type, double sampleRate);
}
