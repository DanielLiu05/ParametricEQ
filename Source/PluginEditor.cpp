/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterDefinitions.h"

//==============================================================================
ParametricEQAudioProcessorEditor::ParametricEQAudioProcessorEditor(ParametricEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    spectrumBackground(p), eqCurve(p)
{
    // Window sizing & resizing constraints
    setSize(850, 600);
    setResizable(true, true);
    setResizeLimits(600, 450, 1600, 1200);

    // 1. Add Visualizers (Order matters! Spectrum first, then Curve on top)
    addAndMakeVisible(spectrumBackground);
    addAndMakeVisible(eqCurve);

    // 2. Build the Bottom Control Panel
    for (int i = 0; i < numBands; ++i)
    {
        auto& bc = bandControls[i];
        juce::String prefix = "band" + juce::String(i) + "_";

        // --- Header (Title & Bypass) ---
        bc.titleLabel.setText("Band " + juce::String(i + 1), juce::dontSendNotification);
        bc.titleLabel.setJustificationType(juce::Justification::centredLeft);
        bc.titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
        bc.titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(bc.titleLabel);

        bc.bypassButton.setButtonText("BYP");
        bc.bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
        bc.bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.getAPVTS(), prefix + "bypass", bc.bypassButton);
        addAndMakeVisible(bc.bypassButton);

        // --- Type ComboBox ---
        // JUCE ComboBox IDs must start at 1. The Attachment handles the 0-based index mapping automatically.
        for (int t = 0; t < static_cast<int>(ParamDefs::BandType::NumTypes); ++t)
            bc.typeBox.addItem(ParamDefs::bandTypeToString(static_cast<ParamDefs::BandType>(t)), t + 1);

        bc.typeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.getAPVTS(), prefix + "type", bc.typeBox);
        addAndMakeVisible(bc.typeBox);

        // --- Sliders Helper Lambda ---
        auto setupSlider = [](juce::Slider& slider, juce::Slider::SliderStyle style)
            {
                slider.setSliderStyle(style);
                slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
                slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
                slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentWhite);
                slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(60, 140, 220)); // Pro Audio Blue
                slider.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(60, 140, 220));
            };

        // --- Frequency (Rotary) ---
        setupSlider(bc.freqSlider, juce::Slider::RotaryHorizontalVerticalDrag);
        bc.freqAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), prefix + "freq", bc.freqSlider);
        addAndMakeVisible(bc.freqSlider);

        // --- Gain (Vertical Linear) ---
        setupSlider(bc.gainSlider, juce::Slider::LinearVertical);
        bc.gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        bc.gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), prefix + "gain", bc.gainSlider);
        addAndMakeVisible(bc.gainSlider);

        // --- Q (Rotary) ---
        setupSlider(bc.qSlider, juce::Slider::RotaryHorizontalVerticalDrag);
        bc.qAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), prefix + "q", bc.qSlider);
        addAndMakeVisible(bc.qSlider);
    }
}

ParametricEQAudioProcessorEditor::~ParametricEQAudioProcessorEditor()
{
}

//==============================================================================
void ParametricEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Dark modern background matching the Spectrum Analyzer
    g.fillAll(juce::Colour::fromRGB(24, 26, 32));

    // Draw a subtle separator line between the graph and the controls
    auto bounds = getLocalBounds();
    auto topArea = bounds.removeFromTop(bounds.getHeight() * 0.65);
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(0.0f, (float)topArea.getBottom(), (float)getWidth(), (float)topArea.getBottom(), 1.0f);
}

void ParametricEQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // 1. Top 65% for Spectrum and Curve (Overlapping)
    auto topArea = bounds.removeFromTop(bounds.getHeight() * 0.65);
    spectrumBackground.setBounds(topArea);
    eqCurve.setBounds(topArea);

    // 2. Bottom 35% for Band Controls
    auto bottomArea = bounds.reduced(15, 10);
    int bandWidth = bottomArea.getWidth() / numBands;

    for (int i = 0; i < numBands; ++i)
    {
        auto& bc = bandControls[i];
        auto bandArea = bottomArea.removeFromLeft(bandWidth).reduced(5);

        // Header Row (Title on left, Bypass on right)
        auto header = bandArea.removeFromTop(24);
        bc.titleLabel.setBounds(header.removeFromLeft(header.getWidth() - 50));
        bc.bypassButton.setBounds(header);

        // Type ComboBox
        auto typeArea = bandArea.removeFromTop(28);
        bc.typeBox.setBounds(typeArea);

        // Sliders Area Padding
        bandArea.removeFromTop(10);

        // Layout: Freq (Left) | Gain (Middle) | Q (Right)
        int sideWidth = bandArea.getWidth() / 3;

        auto freqArea = bandArea.removeFromLeft(sideWidth);
        bc.freqSlider.setBounds(freqArea);

        auto qArea = bandArea.removeFromRight(sideWidth);
        bc.qSlider.setBounds(qArea);

        auto gainArea = bandArea; // Whatever is left in the middle
        bc.gainSlider.setBounds(gainArea);
    }
}