#pragma once

#include <juce_dsp/juce_dsp.h>

class DynamicBand
{
public:
    DynamicBand (float bandQ, float attackMs, float releaseMs,
                 float thresholdDbToUse, float ratioToUse, float maxBoostDbToUse);

    void prepare (double sampleRateToUse);
    void reset();

    void process (const juce::AudioBuffer<float>& dryInput,
                  juce::AudioBuffer<float>& outputToAddTo,
                  juce::AudioBuffer<float>* monoContribution,
                  int numChannels, int numSamples,
                  float centreFrequencyHz, float amount01);

private:
    const float q;
    const float attackSeconds;
    const float releaseSeconds;
    const float thresholdDb;
    const float ratio;
    const float maxBoostDb;

    double sampleRate = 44100.0;
    float attackCoeff = 0.5f;
    float releaseCoeff = 0.01f;
    float maxBoostGain = 1.0f;

    float smoothedFrequency = 1000.0f;
    juce::SmoothedValue<float> smoothedAmountGain;
    juce::dsp::IIR::Filter<float> filters[2][2];
    float envelope = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicBand)
};
