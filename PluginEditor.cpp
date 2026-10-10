#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
void KnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider& slider)
{
    const auto accentColour = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto trackColour  = slider.findColour (juce::Slider::rotarySliderOutlineColourId);

    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const auto lineWidth = radius * 0.12f;
    const auto arcRadius = radius - lineWidth * 0.5f;
    const juce::PathStrokeType stroke (lineWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (trackColour);
    g.strokePath (track, stroke);

    if (angle > rotaryStartAngle)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, angle, true);
        g.setColour (accentColour);
        g.strokePath (valueArc, stroke);
    }

    const auto knobRadius = arcRadius - lineWidth * 1.4f;
    const auto knobBounds = juce::Rectangle<float> (knobRadius * 2.0f, knobRadius * 2.0f).withCentre (centre);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff34373e), centre.x, knobBounds.getY(),
                                             juce::Colour (0xff1c1e23), centre.x, knobBounds.getBottom(), false));
    g.fillEllipse (knobBounds);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawEllipse (knobBounds, 1.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -knobRadius * 0.9f, 3.0f, knobRadius * 0.5f, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

    g.setColour (accentColour);
    g.fillPath (pointer);
}

juce::Label* KnobLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = juce::LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (juce::Font (juce::FontOptions (16.0f)));
    return label;
}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      gainAttachment (p.parameters, "gain", gainSlider)
{
    juce::ignoreUnused (processorRef);

    gainSlider.setLookAndFeel (&knobLookAndFeel);
    gainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 28);
    gainSlider.setTextValueSuffix (" dB");

    gainSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff4fb8f5));
    gainSlider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff363a42));
    gainSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.85f));
    gainSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    gainSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    addAndMakeVisible (gainSlider);

    setSize (300, 330);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    gainSlider.setLookAndFeel (nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff20232a), 0.0f, 0.0f,
                                             juce::Colour (0xff111216), 0.0f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.setFont (juce::Font (juce::FontOptions (18.0f).withStyle ("Bold")));
    g.drawText ("ParametricAir", getLocalBounds().removeFromTop (54), juce::Justification::centred);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (54);

    gainSlider.setBounds (area.reduced (20));
}
