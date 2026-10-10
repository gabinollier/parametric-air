#include "LoudnessMeter.h"

void LoudnessMeter::prepare (double sampleRateToUse)
{
    sampleRate = sampleRateToUse;

    const auto shelfCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, 1681.9745, 0.7071752, juce::Decibels::decibelsToGain (3.99984f));

    const auto highPassCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (
        sampleRate, 38.13547, 0.500327);

    for (auto& channel : channels)
    {
        channel.shelf.coefficients = shelfCoefficients;
        channel.highPass.coefficients = highPassCoefficients;
    }

    reset();
}

void LoudnessMeter::reset()
{
    for (auto& channel : channels)
    {
        channel.shelf.reset();
        channel.highPass.reset();
    }

    sumOfSquares = 0.0;
    numSamplesProcessed = 0;
}

void LoudnessMeter::process (const juce::AudioBuffer<float>& buffer, int numChannels, int numSamples)
{
    const auto channelsToProcess = juce::jmin (numChannels, (int) channels.size(), buffer.getNumChannels());

    for (int channel = 0; channel < channelsToProcess; ++channel)
    {
        const auto* data = buffer.getReadPointer (channel);
        auto& filters = channels[(size_t) channel];

        for (int n = 0; n < numSamples; ++n)
        {
            auto sample = filters.shelf.processSample (data[n]);
            sample = filters.highPass.processSample (sample);
            sumOfSquares += (double) sample * sample;
        }
    }

    numSamplesProcessed += (juce::int64) channelsToProcess * (juce::int64) numSamples;
}

float LoudnessMeter::getLoudnessDb() const
{
    if (numSamplesProcessed <= 0)
        return -100.0f;

    const auto meanSquare = juce::jmax (1.0e-12, sumOfSquares / (double) numSamplesProcessed);
    return (float) (10.0 * std::log10 (meanSquare));
}

bool LoudnessMeter::hasEnoughSamples() const
{
    return numSamplesProcessed > (juce::int64) (sampleRate * 0.5);
}
