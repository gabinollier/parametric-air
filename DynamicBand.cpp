#include "DynamicBand.h"

namespace
{
    // Compensation applied to cascaded sections so the overall response keeps
    // the requested bandwidth.
    constexpr float cascadeQCompensation = 0.64f;
}

DynamicBand::DynamicBand (float bandQ, float attackMs, float releaseMs,
                          float thresholdDbToUse, float ratioToUse, float maxBoostDbToUse)
    : q (bandQ),
      attackSeconds (attackMs * 0.001f),
      releaseSeconds (releaseMs * 0.001f),
      thresholdDb (thresholdDbToUse),
      ratio (ratioToUse),
      maxBoostDb (maxBoostDbToUse)
{
}

void DynamicBand::prepare (double sampleRateToUse)
{
    sampleRate = sampleRateToUse;

    attackCoeff  = 1.0f - std::exp (-1.0f / (float) (sampleRate * attackSeconds));
    releaseCoeff = 1.0f - std::exp (-1.0f / (float) (sampleRate * releaseSeconds));

    maxBoostGain = juce::Decibels::decibelsToGain (maxBoostDb);

    smoothedAmountGain.reset (sampleRate, 0.02);
    reset();
}

void DynamicBand::reset()
{
    for (auto& channel : filters)
        for (auto& section : channel)
            section.reset();

    envelope = 0.0f;
    smoothedAmountGain.setCurrentAndTargetValue (smoothedAmountGain.getTargetValue());
}

void DynamicBand::process (const juce::AudioBuffer<float>& dryInput,
                           juce::AudioBuffer<float>& outputToAddTo,
                           juce::AudioBuffer<float>* monoContribution,
                           int numChannels, int numSamples,
                           float centreFrequencyHz, float amount01)
{
    if (numChannels <= 0 || numSamples <= 0)
        return;

    smoothedFrequency += (centreFrequencyHz - smoothedFrequency) * 0.25f;

    const auto coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass (
        sampleRate, smoothedFrequency, q * cascadeQCompensation);

    for (auto& channel : filters)
        for (auto& section : channel)
            section.coefficients = coefficients;

    smoothedAmountGain.setTargetValue (maxBoostGain * std::pow (juce::jlimit (0.0f, 1.0f, amount01), 0.75f));

    const auto* in0 = dryInput.getReadPointer (0);
    const auto* in1 = dryInput.getReadPointer (numChannels > 1 ? 1 : 0);
    auto* out0 = outputToAddTo.getWritePointer (0);
    auto* out1 = numChannels > 1 && outputToAddTo.getNumChannels() > 1
                     ? outputToAddTo.getWritePointer (1)
                     : nullptr;
    auto* contribution = monoContribution != nullptr ? monoContribution->getWritePointer (0) : nullptr;

    auto& filter0a = filters[0][0];
    auto& filter0b = filters[0][1];
    auto& filter1a = filters[1][0];
    auto& filter1b = filters[1][1];

    for (int n = 0; n < numSamples; ++n)
    {
        const auto band0 = filter0b.processSample (filter0a.processSample (in0[n]));
        const auto band1 = filter1b.processSample (filter1a.processSample (in1[n]));

        const auto rectified = 0.5f * (std::abs (band0) + std::abs (band1));
        envelope += (rectified - envelope) * (rectified > envelope ? attackCoeff : releaseCoeff);

        const auto levelDb = juce::Decibels::gainToDecibels (envelope, -120.0f);
        const auto overDb = juce::jmax (0.0f, levelDb - thresholdDb);
        const auto compressionGain = juce::Decibels::decibelsToGain (-overDb * (1.0f - 1.0f / ratio));
        const auto gain = smoothedAmountGain.getNextValue() * compressionGain;

        out0[n] += band0 * gain;

        if (out1 != nullptr)
            out1[n] += band1 * gain;

        if (contribution != nullptr)
            contribution[n] += 0.5f * (band0 + band1) * gain;
    }
}
