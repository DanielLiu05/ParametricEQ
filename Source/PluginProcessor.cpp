#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "APVTSBuilder.h"
#include "ParametricBand.h"

//==============================================================================
ParametricEQAudioProcessor::ParametricEQAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", APVTSBuilder::createParameters())
{
    // Register as APVTS listener for parameter changes (message thread)
    apvts.addParameterListener(this);

    // Initialize smoothed values and atomic targets with defaults
    for (int i = 0; i < numBands; ++i)
    {
        auto& sp = smoothedParams[i];
        float defaultFreq = ParamDefs::getBandDefaultFreq(i);

        sp.freq.setTargetValue(defaultFreq);
        sp.gain.setTargetValue(0.0f);
        sp.q.setTargetValue(1.0f);

        // Initialize atomic targets
        targetFreq[i].store(defaultFreq);
        targetGain[i].store(0.0f);
        targetQ[i].store(1.0f);
        targetType[i].store(static_cast<float>(ParamDefs::BandType::Bell));
        targetBypass[i].store(0.0f);
    }

    // Initialize spectrum analyzer
    spectrumAnalyzer.setFFTSize(2048);
    spectrumAnalyzer.setUpdateRateHz(45);
}

ParametricEQAudioProcessor::~ParametricEQAudioProcessor()
{
    apvts.removeParameterListener(this);
}

//==============================================================================
void ParametricEQAudioProcessor::setupParameters()
{
    // Handled by APVTSBuilder
}

//==============================================================================
void ParametricEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Configure smoothing ramps: 50ms = good balance for zipper noise vs responsiveness
    const int rampLength = static_cast<int>(sampleRate * 0.05);
    for (auto& sp : smoothedParams)
    {
        sp.freq.reset(sampleRate, rampLength);
        sp.gain.reset(sampleRate, rampLength);
        sp.q.reset(sampleRate, rampLength);
    }

    // Prepare DSP chain
    eqChain.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 });

    // Prepare spectrum analyzer
    spectrumAnalyzer.prepare(sampleRate, samplesPerBlock);

    // Force coefficient recalc on first block
    for (auto& dirty : coefficientsDirty)
        dirty = true;

    curveCacheValid.store(false);
}

void ParametricEQAudioProcessor::releaseResources()
{
    eqChain.releaseResources();
    spectrumAnalyzer.releaseResources();
}

bool ParametricEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& inputSet = layouts.getMainInputChannelSet();
    const auto& outputSet = layouts.getMainOutputChannelSet();
    if (inputSet != outputSet)
        return false;

    return (inputSet == juce::AudioChannelSet::mono()) ||
        (inputSet == juce::AudioChannelSet::stereo());
}

//==============================================================================
void ParametricEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    //==========================================================================
    // 1. Update DSP Coefficients (Block-level smoothing)
    for (int band = 0; band < numBands; ++band)
    {
        auto& sp = smoothedParams[band];

        // Check if continuous parameters are still ramping
        bool isSmoothing = sp.freq.isSmoothing() || sp.gain.isSmoothing() || sp.q.isSmoothing();

        if (isSmoothing || coefficientsDirty[band])
        {
            // Advance the smoothers to the end of the current block
            sp.freq.skip(buffer.getNumSamples());
            sp.gain.skip(buffer.getNumSamples());
            sp.q.skip(buffer.getNumSamples());

            // Read discrete params directly from Atomics (No smoothing to prevent pops!)
            int type = static_cast<int>(targetType[band].load());
            bool bypassed = targetBypass[band].load() > 0.5f;

            eqChain.setBandBypass(band, bypassed);

            if (!bypassed)
            {
                eqChain.updateBandCoefficients(band,
                    sp.freq.getCurrentValue(),
                    sp.gain.getCurrentValue(),
                    sp.q.getCurrentValue(),
                    static_cast<ParamDefs::BandType>(type),
                    currentSampleRate);
            }

            coefficientsDirty[band] = false;
        }
    }

    //==========================================================================
    // 2. Process Audio
    juce::dsp::AudioBlock<float> block(buffer);
    eqChain.process(juce::dsp::ProcessContextReplacing<float>(block));

    //==========================================================================
    // 3. Capture Spectrum (Ensure SpectrumAnalyzer uses lock-free ring buffer internally)
    captureSpectrum(buffer);

    //==========================================================================
    // 4. Invalidate UI Curve Cache (Thread-safe atomic write)
    if (curveRecalculationRequested.exchange(false))
    {
        curveCacheValid.store(false);
    }
}

//==============================================================================
void ParametricEQAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    int bandIndex = -1;
    ParamDefs::ParamType paramType = ParamDefs::ParamType::Unknown;

    if (!ParamDefs::parseParameterID(parameterID, bandIndex, paramType))
        return;

    if (bandIndex < 0 || bandIndex >= numBands)
        return;

    auto& sp = smoothedParams[bandIndex];

    switch (paramType)
    {
    case ParamDefs::ParamType::Frequency:
        sp.freq.setTargetValue(newValue);
        targetFreq[bandIndex].store(newValue);
        break;
    case ParamDefs::ParamType::Gain:
        sp.gain.setTargetValue(newValue);
        targetGain[bandIndex].store(newValue);
        break;
    case ParamDefs::ParamType::Q:
        sp.q.setTargetValue(newValue);
        targetQ[bandIndex].store(newValue);
        break;
    case ParamDefs::ParamType::Type:
        targetType[bandIndex].store(newValue); // Discrete: stored in atomic
        break;
    case ParamDefs::ParamType::Bypass:
        targetBypass[bandIndex].store(newValue); // Discrete: stored in atomic
        break;
    default:
        break;
    }

    // Mark coefficients dirty for this band so the audio thread updates them on the next block
    coefficientsDirty[bandIndex] = true;

    // Request UI curve update
    curveRecalculationRequested.store(true);
}

//==============================================================================
void ParametricEQAudioProcessor::captureSpectrum(const juce::AudioBuffer<float>& buffer)
{
    spectrumAnalyzer.pushAudioBuffer(buffer);
}

//==============================================================================
void ParametricEQAudioProcessor::requestCurveRecalculation()
{
    curveRecalculationRequested.store(true);
}

bool ParametricEQAudioProcessor::getEQCurveData(std::vector<juce::Point<float>>& outPoints) const
{
    // UI thread: return cached curve if valid
    if (curveCacheValid.load() && !cachedCurvePoints.empty())
    {
        outPoints = cachedCurvePoints;
        return true;
    }

    // Compute curve: 300 log-spaced points from 20Hz-20kHz
    constexpr int numPoints = 300;
    constexpr float minFreq = 20.0f;
    constexpr float maxFreq = 20000.0f;

    outPoints.clear();
    outPoints.reserve(numPoints);

    for (int i = 0; i < numPoints; ++i)
    {
        // Logarithmic frequency spacing (Replaced non-existent juce::mapFromLog10)
        const float norm = static_cast<float>(i) / (numPoints - 1);
        const float freq = minFreq * std::pow(maxFreq / minFreq, norm);

        float totalGainDB = 0.0f;

        for (int band = 0; band < numBands; ++band)
        {
            // Read target values directly from Atomics for instant, zero-latency UI feedback
            bool bypassed = targetBypass[band].load() > 0.5f;
            if (bypassed) continue;

            const float f = targetFreq[band].load();
            const float g = targetGain[band].load();
            const float q = targetQ[band].load();
            const int type = static_cast<int>(targetType[band].load());

            totalGainDB += ParametricBand::computeMagnitudeAtFrequency(
                freq, f, g, q, static_cast<ParamDefs::BandType>(type), currentSampleRate);
        }

        outPoints.emplace_back(freq, totalGainDB);
    }

    // Cache result (Safe because getEQCurveData is only called from the UI thread)
    cachedCurvePoints = outPoints;
    curveCacheValid.store(true);

    return true;
}

//==============================================================================
void ParametricEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    std::unique_ptr<juce::XmlElement> xml(apvts.copyState().createXml());
    copyXmlToBinary(*xml, destData);
}

void ParametricEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}