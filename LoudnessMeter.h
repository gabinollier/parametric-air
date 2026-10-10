#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>

class LoudnessMeter
{
public:
    void prepare (double sampleRateToUse);
    void reset();

    void process (const juce::AudioBuffer<float>& buffer, int numChannels, int numSamples);

    float getLoudnessDb() const;
    bool hasEnoughSamples() const;

private:
    struct ChannelFilters
    {
        juce::dsp::IIR::Filter<float> shelf;
        juce::dsp::IIR::Filter<float> highPass;
    };

    std::array<ChannelFilters, 2> channels;
    double sumOfSquares = 0.0;
    juce::int64 numSamplesProcessed = 0;
    double sampleRate = 44100.0;
};
