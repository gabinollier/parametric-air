#pragma once

#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

class SpectrumDisplay final : public juce::Component,
                              private juce::Timer
{
public:
    explicit SpectrumDisplay (AudioPluginAudioProcessor& processor);
    ~SpectrumDisplay() override = default;

    void paint (juce::Graphics&) override;

private:
    static constexpr int fftOrder = 11;
    static constexpr int numColumns = 512;
    static constexpr float minFrequency = 20.0f;
    static constexpr float maxFrequency = 20000.0f;
    static constexpr float minDecibels = -90.0f;
    static constexpr float maxDecibels = 6.0f;
    static constexpr float decayDbPerSecond = 36.0f;

    void timerCallback() override;

    void updateLevels (const std::array<float, SpectrumBuffer::fftSize>& samples,
                       std::array<float, numColumns>& levels);

    void drawGrid (juce::Graphics&) const;
    void drawLegend (juce::Graphics&) const;
    void drawLevels (juce::Graphics&, const std::array<float, numColumns>& levels,
                     juce::Colour lineColour, juce::Colour fillColour) const;
    void drawBandShape (juce::Graphics&, const DynamicBand& band, float amount01,
                        juce::Colour colour) const;

    float getDisplayMaxFrequency() const;
    float xForFrequency (float frequency) const;
    float frequencyForX (float x) const;
    float yForDecibels (float decibels) const;

    AudioPluginAudioProcessor& processorRef;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { SpectrumBuffer::fftSize,
                                                 juce::dsp::WindowingFunction<float>::hann };
    std::array<float, SpectrumBuffer::fftSize * 2> fftData {};
    std::array<float, SpectrumBuffer::fftSize> inputSamples {};
    std::array<float, SpectrumBuffer::fftSize> addedSamples {};
    std::array<float, numColumns> inputLevels {};
    std::array<float, numColumns> addedLevels {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumDisplay)
};
