/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ParameterDefinitions.h"
#include "SpectrumBackground.h"
#include "EQCurveComponent.h"

//==============================================================================
/**
*/
class ParametricEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ParametricEQAudioProcessorEditor (ParametricEQAudioProcessor&);
    ~ParametricEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ParametricEQAudioProcessor& audioProcessor;

    // Visualizers
    SpectrumBackground spectrumBackground;
    EQCurveComponent eqCurve;

    // Band Controls
    struct BandControls
    {
        juce::Label titleLabel;
        juce::ToggleButton bypassButton;
        juce::ComboBox typeBox;
        juce::Slider freqSlider;
        juce::Slider gainSlider;
        juce::Slider qSlider;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttach;
    };

    BandControls bandControls[numBands];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametricEQAudioProcessorEditor)
};
