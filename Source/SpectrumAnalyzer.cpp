/*
  ==============================================================================

    SpectrumAnalyzer.cpp
    Created: 15 May 2026 12:35:57am
    Author:  Daniel

  ==============================================================================
*/

#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer() {}
SpectrumAnalyzer::~SpectrumAnalyzer() {}

void SpectrumAnalyzer::setFFTSize(int newSize)
{
    juce::ScopedLock sl(setupLock);
    fftSize = newSize;
    updateFFTParameters();

    // Reallocate buffers if prepare() was already called
    if (currentSampleRate > 0.0)
    {
        ringBuffer.resize(fftSize, 0.0f);
        windowBuffer.resize(fftSize, 0.0f);
        complexBuffer.resize(fftSize);

        for (auto& frame : frames)
            frame.magnitudes.resize(fftSize / 2 + 1, -120.0f);

        writeIndex = 0;
        numAccumulated = 0;
        samplesSinceLastFFT = 0;
    }
}

void SpectrumAnalyzer::setUpdateRateHz(int newRate)
{
    juce::ScopedLock sl(setupLock);
    updateRateHz = newRate;
    if (currentSampleRate > 0.0)
        samplesBetweenFFT = static_cast<int>(currentSampleRate / updateRateHz);
}

void SpectrumAnalyzer::prepare(double sampleRate, int samplesPerBlock)
{
    juce::ScopedLock sl(setupLock);
    currentSampleRate = sampleRate;
    samplesBetweenFFT = static_cast<int>(currentSampleRate / updateRateHz);

    ringBuffer.resize(fftSize, 0.0f);
    windowBuffer.resize(fftSize, 0.0f);
    complexBuffer.resize(fftSize);

    for (auto& frame : frames)
    {
        frame.magnitudes.resize(fftSize / 2 + 1, -120.0f);
        frame.numBins = 0;
    }

    writeIndex = 0;
    numAccumulated = 0;
    samplesSinceLastFFT = 0;

    updateFFTParameters();
}

void SpectrumAnalyzer::releaseResources()
{
    juce::ScopedLock sl(setupLock);
    ringBuffer.clear();
    windowBuffer.clear();
    complexBuffer.clear();
    for (auto& frame : frames)
        frame.magnitudes.clear();

    fft.reset();
    window.reset(); // <-- 添加：释放 window 资源

    writeIndex = 0;
    numAccumulated = 0;
    samplesSinceLastFFT = 0;
}

void SpectrumAnalyzer::updateFFTParameters()
{
    fftOrder = static_cast<int>(std::log2(fftSize));
    // 修复了原有的 make_unique 语法错误
    fft = std::make_unique<juce::dsp::FFT>(fftOrder);
    // 正确构造 WindowingFunction，传入大小和类型
    window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::hann);
}

void SpectrumAnalyzer::pushAudioBuffer(const juce::AudioBuffer<float>& buffer)
{
    if (fftSize == 0 || currentSampleRate <= 0.0) return;

    // Analyze Left Channel (or Mono)
    const float* channelData = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();

    // 1. Push to Circular Ring Buffer
    for (int i = 0; i < numSamples; ++i)
    {
        ringBuffer[writeIndex] = channelData[i];
        writeIndex = (writeIndex + 1) % fftSize;
    }

    // Prime the buffer on startup
    if (numAccumulated < fftSize)
        numAccumulated = juce::jmin(numAccumulated + numSamples, fftSize);

    samplesSinceLastFFT += numSamples;

    // 2. Trigger FFT if we have enough history AND enough time has passed
    // 修复了原有的 "& &" 语法错误
    if (numAccumulated >= fftSize && samplesSinceLastFFT >= samplesBetweenFFT)
    {
        // Snapshot the last fftSize samples from the circular buffer
        int readIndex = writeIndex;
        for (int i = 0; i < fftSize; ++i)
        {
            windowBuffer[i] = ringBuffer[readIndex];
            readIndex = (readIndex + 1) % fftSize;
        }

        // Apply Hann Window (修复了原有的 "Windowi ngTable" 拼写错误，并使用 -> 调用)
        window->multiplyWithWindowingTable(windowBuffer.data(), fftSize);

        // Load into Complex Buffer for FFT
        for (int i = 0; i < fftSize; ++i)
            complexBuffer[i] = std::complex<float>(windowBuffer[i], 0.0f);

        // Perform FFT
        fft->perform(complexBuffer.data(), complexBuffer.data(), false);

        // Calculate Magnitudes & store in BACK buffer
        auto& backFrame = frames[backFrameIndex];
        backFrame.numBins = fftSize / 2 + 1;

        for (int i = 0; i < backFrame.numBins; ++i)
        {
            float mag = std::abs(complexBuffer[i]) / (float)fftSize;
            // Convert to dB, clamp floor to -120dB to prevent -inf (修复了 "m agnitudes" 拼写错误)
            backFrame.magnitudes[i] = juce::Decibels::gainToDecibels(mag, -120.0f);
        }

        // Atomic Swap: Make the back buffer the new active buffer for the UI (修复了 "backFrameIn dex" 拼写错误)
        activeFrameIndex.store(backFrameIndex);
        backFrameIndex = (backFrameIndex + 1) % 2;

        // Reset Timer
        samplesSinceLastFFT = 0;
    }
}

void SpectrumAnalyzer::getLatestSpectrum(std::vector<float>& outMagnitudes) const
{
    // Read from the currently active frame (Lock-free)
    int readIndex = activeFrameIndex.load();
    const auto& frame = frames[readIndex];
    outMagnitudes.resize(frame.numBins);
    for (int i = 0; i < frame.numBins; ++i)
    {
        outMagnitudes[i] = frame.magnitudes[i];
    }
}