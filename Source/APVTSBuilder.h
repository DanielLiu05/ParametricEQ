/*
  ==============================================================================

    APVTSBuilder.h
    Created: 15 May 2026 12:42:54am
    Author:  Daniel

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "ParameterDefinitions.h"

namespace APVTSBuilder
{
    /**
     * Creates the parameter layout for the AudioProcessorValueTreeState.
     * Generates 5 bands of (Frequency, Gain, Q, Type, Bypass).
     */
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
}
