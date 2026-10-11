#pragma once

#include "PluginProcessor.h"
#include "SpectrumDisplay.h"

//==============================================================================
class KnobLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;
};

//==============================================================================
class ControlPanel final : public juce::Component,
                           private juce::Timer
{
public:
    explicit ControlPanel (AudioPluginAudioProcessor&);
    ~ControlPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int preferredHeight = 166;

private:
    static constexpr int shadowSize = 10;
    static constexpr float cornerRadius = 14.0f;

    void timerCallback() override;

    void setupKnob (juce::Slider&, juce::Colour accent);
    void setupCaption (juce::Label&, const juce::String& text, juce::Colour colour, bool bold);
    void layoutKnobGroup (juce::Rectangle<int> area, juce::Label& title,
                          juce::Slider& firstKnob, juce::Label& firstCaption,
                          juce::Slider& secondKnob, juce::Label& secondCaption);

    AudioPluginAudioProcessor& processorRef;

    KnobLookAndFeel knobLookAndFeel;

    juce::Slider presenceAmountKnob, presenceFreqKnob;
    juce::Slider airAmountKnob, airFreqKnob;
    juce::Slider trimKnob;

    juce::AudioProcessorValueTreeState::SliderAttachment presenceAmountAttachment, presenceFreqAttachment,
                                                  airAmountAttachment, airFreqAttachment,
                                                  trimAttachment;

    juce::Label presenceTitle, presenceAmountCaption, presenceFreqCaption;
    juce::Label airTitle, airAmountCaption, airFreqCaption;
    juce::Label outputTitle, trimCaption;

    juce::TextButton matchButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlPanel)
};

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    SpectrumDisplay spectrum;
    ControlPanel controlPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
