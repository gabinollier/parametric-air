#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

#include "DynamicBand.h"
#include "LoudnessMeter.h"
#include "SpectrumBuffer.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor,
                                        private juce::AsyncUpdater
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;

    SpectrumBuffer& getInputSpectrum() noexcept     { return inputSpectrum; }
    SpectrumBuffer& getAddedSpectrum() noexcept     { return addedSpectrum; }
    const DynamicBand& getPresenceBand() const noexcept { return presenceBand; }
    const DynamicBand& getAirBand() const noexcept      { return airBand; }

    void requestGainMatch();
    bool isMatching() const noexcept                { return matchRunning.load(); }

private:
    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void handleAsyncUpdate() override;

    DynamicBand presenceBand, airBand;

    juce::SmoothedValue<float> outputGain;
    juce::AudioBuffer<float> dryScratch, addedScratch;

    std::atomic<float>* presenceAmountParameter = nullptr;
    std::atomic<float>* presenceFrequencyParameter = nullptr;
    std::atomic<float>* airAmountParameter = nullptr;
    std::atomic<float>* airFrequencyParameter = nullptr;
    std::atomic<float>* trimParameterValue = nullptr;
    juce::RangedAudioParameter* trimParameter = nullptr;

    SpectrumBuffer inputSpectrum, addedSpectrum;
    LoudnessMeter inputLoudness, outputLoudness;

    std::atomic<bool> matchRequested { false };
    std::atomic<bool> matchRunning { false };
    std::atomic<float> pendingTrimDb { 0.0f };
    int matchSamplesRemaining = 0;
    int matchWarmupRemaining = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
