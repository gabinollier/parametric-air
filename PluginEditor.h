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
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    void setupKnob (juce::Slider&, juce::Colour accent);
    void setupCaption (juce::Label&, const juce::String& text, juce::Colour colour, bool bold);
    void layoutKnobGroup (juce::Rectangle<int> area, juce::Label& title,
                          juce::Slider& firstKnob, juce::Label& firstCaption,
                          juce::Slider& secondKnob, juce::Label& secondCaption);

    AudioPluginAudioProcessor& processorRef;

    KnobLookAndFeel knobLookAndFeel;
    SpectrumDisplay spectrum;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
