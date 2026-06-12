/*
  ==============================================================================

    SpectrumAnalyzer.h
    Created: 15 May 2026 12:35:57am
    Author:  Daniel

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <array>
#include <atomic>
#include <complex>
#include <cmath>

/**
A real-time safe, background FFT spectrum analyzer.
Uses a circular buffer for audio accumulation and lock-free double-buffering
to safely pass magnitude data to the UI thread without mutexes.
*/
class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();
    ~SpectrumAnalyzer();

    /** Configuration (Call on Message Thread before prepare) */
    void setFFTSize(int newSize);
    void setUpdateRateHz(int newRate);

    /** Lifecycle */
    void prepare(double sampleRate, int samplesPerBlock);
    void releaseResources();

    /**
    Audio Thread (Real-Time Safe).
    Pushes incoming audio into the circular buffer and triggers FFT when needed.
    */
    void pushAudioBuffer(const juce::AudioBuffer<float>& buffer);

    /**
    UI Thread.
    Copies the latest calculated spectrum into outMagnitudes.
    outMagnitudes will be resized to match the number of FFT bins.
    */
    void getLatestSpectrum(std::vector<float>& outMagnitudes) const;

private:
    void updateFFTParameters();

    // Configuration
    int fftSize = 2048;
    int fftOrder = 11;
    int updateRateHz = 45;
    double currentSampleRate = 44100.0;

    // DSP Objects
    std::unique_ptr<juce::dsp::FFT> fft;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;                //     改为 unique_ptr

    // Circular Accumulation Buffer
    std::vector<float> ringBuffer;
    int writeIndex = 0;
    int numAccumulated = 0;

    // Rate Limiting
    int samplesSinceLastFFT = 0;
    int samplesBetweenFFT = 0;

    // Processing Scratch Buffers (Pre-allocated)
    std::vector<float> windowBuffer;
    std::vector<std::complex<float>> complexBuffer;

    // Lock-Free Double Buffering for UI Thread
    struct SpectrumFrame
    {
        std::vector<float> magnitudes; // Pre-allocated to max size
        int numBins = 0;
    };

    std::array<SpectrumFrame, 2> frames;
    std::atomic<int> activeFrameIndex{ 0 };
    int backFrameIndex = 1;

    juce::CriticalSection setupLock; // ONLY used on Message Thread during prepare/setup
};