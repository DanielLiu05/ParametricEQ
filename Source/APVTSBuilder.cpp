/*
  ==============================================================================

    APVTSBuilder.cpp
    Created: 15 May 2026 12:33:53am
    Author:  Daniel

  ==============================================================================
*/

#include "APVTSBuilder.h"

namespace APVTSBuilder
{
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // 1. True Logarithmic Range for Frequency (20Hz - 20kHz)
        juce::NormalisableRange<float> freqRange(20.0f, 20000.0f,
            [](float rangeStart, float rangeEnd, float normalised) {
                return rangeStart * std::pow(rangeEnd / rangeStart, normalised);
            },
            [](float rangeStart, float rangeEnd, float value) {
                return std::log(value / rangeStart) / std::log(rangeEnd / rangeStart);
            }
        );

        // 2. True Logarithmic Range for Q (0.1 - 10.0)
        juce::NormalisableRange<float> qRange(0.1f, 10.0f,
            [](float rangeStart, float rangeEnd, float normalised) {
                return rangeStart * std::pow(rangeEnd / rangeStart, normalised);
            },
            [](float rangeStart, float rangeEnd, float value) {
                return std::log(value / rangeStart) / std::log(rangeEnd / rangeStart);
            }
        );

        // 3. Linear Range for Gain (-24dB to +24dB)
        juce::NormalisableRange<float> gainRange(-24.0f, 24.0f, 0.01f);

        // Pre-build the choice array for Filter Types
        juce::StringArray typeNames;
        for (int t = 0; t < static_cast<int>(ParamDefs::BandType::NumTypes); ++t)
            typeNames.add(ParamDefs::bandTypeToString(static_cast<ParamDefs::BandType>(t)));

        for (int i = 0; i < numBands; ++i)
        {
            juce::String bandPrefix = "band" + juce::String(i) + "_";
            juce::String bandName = "Band " + juce::String(i + 1) + " ";

            // Frequency Parameter
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{ bandPrefix + "freq", 1 }, // '1' is the version hint (Required in JUCE 7+)
                bandName + "Freq",
                freqRange,
                ParamDefs::getBandDefaultFreq(i),
                juce::String(),
                juce::AudioProcessorParameter::genericParameter,
                [](float value, int) { return juce::String(value, 1) + " Hz"; },
                [](const juce::String& text) { return text.getFloatValue(); }
            ));

            // Gain Parameter
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{ bandPrefix + "gain", 1 },
                bandName + "Gain",
                gainRange,
                0.0f,
                juce::String(),
                juce::AudioProcessorParameter::genericParameter,
                [](float value, int) { return juce::String(value, 2) + " dB"; },
                [](const juce::String& text) { return text.getFloatValue(); }
            ));

            // Q Parameter
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{ bandPrefix + "q", 1 },
                bandName + "Q",
                qRange,
                1.0f,
                juce::String(),
                juce::AudioProcessorParameter::genericParameter,
                [](float value, int) { return juce::String(value, 2); },
                [](const juce::String& text) { return text.getFloatValue(); }
            ));

            // Type Parameter (Choice)
            // Make Band 1 default to Low Shelf, Band 5 default to High Shelf, others Bell
            int defaultTypeIndex = static_cast<int>(ParamDefs::BandType::Bell);
            if (i == 0) defaultTypeIndex = static_cast<int>(ParamDefs::BandType::LowShelf);
            if (i == numBands - 1) defaultTypeIndex = static_cast<int>(ParamDefs::BandType::HighShelf);

            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID{ bandPrefix + "type", 1 },
                bandName + "Type",
                typeNames,
                defaultTypeIndex
            ));

            // Bypass Parameter (Bool)
            params.push_back(std::make_unique<juce::AudioParameterBool>(
                juce::ParameterID{ bandPrefix + "bypass", 1 },
                bandName + "Bypass",
                false
            ));
        }

        return { params.begin(), params.end() };
    }
}