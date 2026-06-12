/*
  ==============================================================================

    ParameterDefinitions.h
    Created: 15 May 2026 12:33:33am
    Author:  Daniel

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// Must be in global namespace as it's used directly in std::array templates in PluginProcessor.h
constexpr int numBands = 5;

namespace ParamDefs
{
    enum class BandType
    {
        Bell = 0,
        LowShelf,
        HighShelf,
        LowPass,
        HighPass,
        Notch,
        BandPass,
        NumTypes
    };

    enum class ParamType
    {
        Unknown = 0,
        Frequency,
        Gain,
        Q,
        Type,
        Bypass
    };

    // Musically spaced default frequencies for a 5-band EQ
    inline float getBandDefaultFreq(int bandIndex)
    {
        const float defaults[numBands] = { 60.0f, 250.0f, 1000.0f, 3500.0f, 12000.0f };
        if (bandIndex >= 0 && bandIndex < numBands)
            return defaults[bandIndex];
        return 1000.0f;
    }

    // Parses IDs like "band0_freq" into bandIndex and ParamType
    inline bool parseParameterID(const juce::String& parameterID, int& bandIndex, ParamType& paramType)
    {
        if (!parameterID.startsWith("band"))
            return false;

        auto underscoreIndex = parameterID.indexOf("_");
        if (underscoreIndex == -1)
            return false;

        bandIndex = parameterID.substring(4, underscoreIndex).getIntValue();
        if (bandIndex < 0 || bandIndex >= numBands)
            return false;

        juce::String typeStr = parameterID.substring(underscoreIndex + 1);

        if (typeStr == "freq")   paramType = ParamType::Frequency;
        else if (typeStr == "gain")   paramType = ParamType::Gain;
        else if (typeStr == "q")      paramType = ParamType::Q;
        else if (typeStr == "type")   paramType = ParamType::Type;
        else if (typeStr == "bypass") paramType = ParamType::Bypass;
        else                          return false;

        return true;
    }

    inline juce::String bandTypeToString(BandType type)
    {
        switch (type)
        {
        case BandType::Bell:      return "Bell";
        case BandType::LowShelf:  return "Low Shelf";
        case BandType::HighShelf: return "High Shelf";
        case BandType::LowPass:   return "Low Pass";
        case BandType::HighPass:  return "High Pass";
        case BandType::Notch:     return "Notch";
        case BandType::BandPass:  return "Band Pass";
        default:                  return "Unknown";
        }
    }
}
