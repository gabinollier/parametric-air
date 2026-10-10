#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float presenceQ = 1.1f;
    constexpr float airQ = 0.7f;

    constexpr float matchWarmupSeconds = 0.2f;
    constexpr float matchMeasureSeconds = 2.0f;
}

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       parameters (*this, nullptr, "PARAMETERS", createParameterLayout()),
       presenceBand (presenceQ, 10.0f, 150.0f, -42.0f, 3.0f, 18.0f),
       airBand (airQ, 5.0f, 90.0f, -46.0f, 3.0f, 20.0f)
{
    presenceAmountParameter    = parameters.getRawParameterValue ("presenceAmount");
    presenceFrequencyParameter = parameters.getRawParameterValue ("presenceFreq");
    airAmountParameter         = parameters.getRawParameterValue ("airAmount");
    airFrequencyParameter      = parameters.getRawParameterValue ("airFreq");
    trimParameterValue         = parameters.getRawParameterValue ("trim");
    trimParameter              = parameters.getParameter ("trim");
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const auto amountAttributes = juce::AudioParameterFloatAttributes()
        .withLabel ("%")
        .withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (juce::roundToInt (value)) + " %";
        });

    const auto frequencyAttributes = juce::AudioParameterFloatAttributes()
        .withLabel ("Hz")
        .withStringFromValueFunction ([] (float value, int)
        {
            return value < 1000.0f ? juce::String (juce::roundToInt (value)) + " Hz"
                                   : juce::String (value / 1000.0f, 1) + " kHz";
        })
        .withValueFromStringFunction ([] (const juce::String& text)
        {
            const auto value = text.getFloatValue();
            return text.containsIgnoreCase ("k") || value < 100.0f ? value * 1000.0f : value;
        });

    const auto makeFrequencyRange = [] (float minimum, float maximum, float centre)
    {
        juce::NormalisableRange<float> range (minimum, maximum, 1.0f);
        range.setSkewForCentre (centre);
        return range;
    };

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "presenceAmount", 1 },
        "Presence Amount",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f },
        0.0f,
        amountAttributes));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "presenceFreq", 1 },
        "Presence Frequency",
        makeFrequencyRange (800.0f, 6000.0f, 2200.0f),
        3000.0f,
        frequencyAttributes));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "airAmount", 1 },
        "Air Amount",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f },
        0.0f,
        amountAttributes));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "airFreq", 1 },
        "Air Frequency",
        makeFrequencyRange (4000.0f, 18000.0f, 9000.0f),
        11000.0f,
        frequencyAttributes));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "trim", 1 },
        "Trim",
        juce::NormalisableRange<float> { -18.0f, 18.0f, 0.1f },
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withStringFromValueFunction ([] (float value, int)
            {
                return juce::String (value, 1) + " dB";
            })));

    return layout;
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================
int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dryScratch.setSize (2, samplesPerBlock, false, false, true);
    monoScratch.setSize (1, samplesPerBlock, false, false, true);
    presenceScratch.setSize (1, samplesPerBlock, false, false, true);
    airScratch.setSize (1, samplesPerBlock, false, false, true);

    presenceBand.prepare (sampleRate);
    airBand.prepare (sampleRate);

    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (trimParameterValue->load()));

    inputSpectrum.clear();
    presenceSpectrum.clear();
    airSpectrum.clear();

    inputLoudness.prepare (sampleRate);
    outputLoudness.prepare (sampleRate);

    matchRunning = false;
    matchSamplesRemaining = 0;
    matchWarmupRemaining = 0;
}

void AudioPluginAudioProcessor::releaseResources()
{
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    const auto numChannels = juce::jmin (2, totalNumInputChannels, buffer.getNumChannels());

    if (numChannels <= 0 || numSamples <= 0 || numSamples > dryScratch.getNumSamples())
        return;

    for (int channel = 0; channel < numChannels; ++channel)
        dryScratch.copyFrom (channel, 0, buffer, channel, 0, numSamples);

    auto* mono = monoScratch.getWritePointer (0);

    if (numChannels > 1)
    {
        const auto* left = dryScratch.getReadPointer (0);
        const auto* right = dryScratch.getReadPointer (1);

        for (int n = 0; n < numSamples; ++n)
            mono[n] = 0.5f * (left[n] + right[n]);
    }
    else
    {
        std::copy (dryScratch.getReadPointer (0), dryScratch.getReadPointer (0) + numSamples, mono);
    }

    inputSpectrum.push (mono, numSamples);

    if (matchRequested.exchange (false))
    {
        inputLoudness.reset();
        outputLoudness.reset();

        matchWarmupRemaining = (int) (getSampleRate() * matchWarmupSeconds);
        matchSamplesRemaining = (int) (getSampleRate() * matchMeasureSeconds) + matchWarmupRemaining;
        matchRunning = true;
    }

    presenceScratch.clear();
    airScratch.clear();

    presenceBand.process (dryScratch, buffer, &presenceScratch, numChannels, numSamples,
                          presenceFrequencyParameter->load(), presenceAmountParameter->load() * 0.01f);
    airBand.process (dryScratch, buffer, &airScratch, numChannels, numSamples,
                     airFrequencyParameter->load(), airAmountParameter->load() * 0.01f);

    presenceSpectrum.push (presenceScratch.getReadPointer (0), numSamples);
    airSpectrum.push (airScratch.getReadPointer (0), numSamples);

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (trimParameterValue->load()));
    outputGain.applyGain (buffer, numSamples);

    if (matchRunning)
    {
        if (matchWarmupRemaining > 0)
        {
            matchWarmupRemaining -= numSamples;
        }
        else
        {
            inputLoudness.process (dryScratch, numChannels, numSamples);
            outputLoudness.process (buffer, numChannels, numSamples);

            matchSamplesRemaining -= numSamples;

            if (matchSamplesRemaining <= 0)
            {
                matchRunning = false;

                const auto inputDb = inputLoudness.getLoudnessDb();
                const auto outputDb = outputLoudness.getLoudnessDb();

                if (inputLoudness.hasEnoughSamples() && outputLoudness.hasEnoughSamples()
                    && inputDb > -70.0f && outputDb > -70.0f)
                {
                    pendingTrimDb = juce::jlimit (-18.0f, 18.0f,
                                                  trimParameterValue->load() + inputDb - outputDb);
                    triggerAsyncUpdate();
                }
            }
        }
    }
}

void AudioPluginAudioProcessor::handleAsyncUpdate()
{
    if (trimParameter != nullptr)
        trimParameter->setValueNotifyingHost (trimParameter->convertTo0to1 (pendingTrimDb.load()));
}

void AudioPluginAudioProcessor::requestGainMatch()
{
    matchRequested = true;
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState().createXml())
        copyXmlToBinary (*state, destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto state = getXmlFromBinary (data, sizeInBytes))
        if (state->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*state));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
