#include "DynamicBand.h"

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

    smoothedBoostDb.reset (sampleRate, 0.02);
    reset();
}

void DynamicBand::reset()
{
    filters[0].reset();
    filters[1].reset();
    envelope = 0.0f;
    smoothedBoostDb.setCurrentAndTargetValue (smoothedBoostDb.getTargetValue());
}

juce::dsp::IIR::Coefficients<float>::Ptr DynamicBand::getCurrentCoefficients() const
{
    return juce::dsp::IIR::Coefficients<float>::makeBandPass (
        sampleRate, smoothedFrequency.load (std::memory_order_relaxed), q);
}

void DynamicBand::process (const juce::AudioBuffer<float>& dryInput,
                           juce::AudioBuffer<float>& outputToAddTo,
                           juce::AudioBuffer<float>* monoContribution,
                           int numChannels, int numSamples,
                           float centreFrequencyHz, float amount01)
{
    if (numChannels <= 0 || numSamples <= 0)
        return;

    const auto currentFrequency = smoothedFrequency.load (std::memory_order_relaxed);
    smoothedFrequency.store (currentFrequency + (centreFrequencyHz - currentFrequency) * 0.25f,
                             std::memory_order_relaxed);

    const auto coefficients = getCurrentCoefficients();
    filters[0].coefficients = coefficients;
    filters[1].coefficients = coefficients;

    smoothedBoostDb.setTargetValue (maxBoostDb * std::pow (juce::jlimit (0.0f, 1.0f, amount01), 0.75f));

    const auto* in0 = dryInput.getReadPointer (0);
    const auto* in1 = dryInput.getReadPointer (numChannels > 1 ? 1 : 0);
    auto* out0 = outputToAddTo.getWritePointer (0);
    auto* out1 = numChannels > 1 && outputToAddTo.getNumChannels() > 1
                     ? outputToAddTo.getWritePointer (1)
                     : nullptr;
    auto* contribution = monoContribution != nullptr ? monoContribution->getWritePointer (0) : nullptr;

    auto& filter0 = filters[0];
    auto& filter1 = filters[1];

    for (int n = 0; n < numSamples; ++n)
    {
        const auto band0 = filter0.processSample (in0[n]);
        const auto band1 = filter1.processSample (in1[n]);

        const auto rectified = 0.5f * (std::abs (band0) + std::abs (band1));
        envelope += (rectified - envelope) * (rectified > envelope ? attackCoeff : releaseCoeff);

        const auto levelDb = juce::Decibels::gainToDecibels (envelope, -120.0f);
        const auto overDb = juce::jmax (0.0f, levelDb - thresholdDb);
        const auto gainDb = smoothedBoostDb.getNextValue() - overDb * (1.0f - 1.0f / ratio);
        const auto gain = juce::Decibels::decibelsToGain (gainDb);

        out0[n] += band0 * gain;

        if (out1 != nullptr)
            out1[n] += band1 * gain;

        if (contribution != nullptr)
            contribution[n] += 0.5f * (band0 + band1) * gain;
    }
}
