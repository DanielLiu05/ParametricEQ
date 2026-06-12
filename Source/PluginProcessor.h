#pragma once
#include <JuceHeader.h>
#include "ParameterDefinitions.h"
#include "EQChain.h"
#include "SpectrumAnalyzer.h"

//==============================================================================
/**
    Audio processor for a 5-band parametric equalizer.
*/
class ParametricEQAudioProcessor : public juce::AudioProcessor,
    private juce::AudioProcessorValueTreeState::Listener // FIX: Added Listener inheritance
{
public:
    //==============================================================================
    ParametricEQAudioProcessor();
    ~ParametricEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Public accessors for UI & visualization
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Thread-safe spectrum data access (UI calls this)
    const SpectrumAnalyzer& getSpectrumAnalyzer() const { return spectrumAnalyzer; }

    // Request EQ curve recalculation (UI calls when parameters change)
    void requestCurveRecalculation();
    bool getEQCurveData(std::vector<juce::Point<float>>& outPoints) const;

    // ui
    float getTargetFreq(int band) const noexcept { return targetFreq[band].load(); }
    float getTargetGain(int band) const noexcept { return targetGain[band].load(); }
    float getTargetQ(int band) const noexcept { return targetQ[band].load(); }
    float getTargetBypass(int band) const noexcept { return targetBypass[band].load(); }
    double getSampleRate() const noexcept { return currentSampleRate; }

private:
    //==============================================================================
    // Parameter handling
    void setupParameters();
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Spectrum capture (audio thread)
    void captureSpectrum(const juce::AudioBuffer<float>& buffer); // FIX: Typo corrected

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    // DSP chain: 5 parametric bands
    EQChain eqChain; // FIX: Typo corrected

    // Smoothing containers: ONLY for continuous parameters (audio thread)
    struct SmoothedParams {
        juce::SmoothedValue<float> freq;
        juce::SmoothedValue<float> gain;
        juce::SmoothedValue<float> q;
    };
    std::array<SmoothedParams, numBands> smoothedParams;

    // Thread-safe target values for discrete parameters & UI curve drawing
    std::array<std::atomic<float>, numBands> targetType;
    std::array<std::atomic<float>, numBands> targetBypass;
    std::array<std::atomic<float>, numBands> targetFreq;
    std::array<std::atomic<float>, numBands> targetGain;
    std::array<std::atomic<float>, numBands> targetQ;

    // Coefficient update tracking
    std::array<bool, numBands> coefficientsDirty{ true, true, true, true, true };

    // Spectrum analyzer (thread-safe, background FFT)
    SpectrumAnalyzer spectrumAnalyzer;

    // EQ curve caching (for UI)
    mutable std::vector<juce::Point<float>> cachedCurvePoints;
    mutable std::atomic<bool> curveCacheValid{ false }; // FIX: Atomic to prevent data race
    std::atomic<bool> curveRecalculationRequested{ false };

    // Sample rate tracking
    double currentSampleRate{ 44100.0 };


    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParametricEQAudioProcessor)
};