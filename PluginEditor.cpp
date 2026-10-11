#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "Theme.h"

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
    label->setFont (juce::Font (juce::FontOptions (15.0f)));
    return label;
}

//==============================================================================
ControlPanel::ControlPanel (AudioPluginAudioProcessor& p)
    : processorRef (p),
      presenceAmountAttachment (p.parameters, "presenceAmount", presenceAmountKnob),
      presenceFreqAttachment (p.parameters, "presenceFreq", presenceFreqKnob),
      airAmountAttachment (p.parameters, "airAmount", airAmountKnob),
      airFreqAttachment (p.parameters, "airFreq", airFreqKnob),
      trimAttachment (p.parameters, "trim", trimKnob)
{
    setupKnob (presenceAmountKnob, Theme::presence);
    setupKnob (presenceFreqKnob, Theme::presence);
    setupKnob (airAmountKnob, Theme::air);
    setupKnob (airFreqKnob, Theme::air);
    setupKnob (trimKnob, Theme::trim);

    setupCaption (presenceTitle, "PRESENCE", Theme::presence, true);
    setupCaption (airTitle, "AIR", Theme::air, true);
    setupCaption (outputTitle, "OUTPUT", Theme::trim, true);
    setupCaption (presenceAmountCaption, "AMOUNT", Theme::dimText, false);
    setupCaption (presenceFreqCaption, "FREQ", Theme::dimText, false);
    setupCaption (airAmountCaption, "AMOUNT", Theme::dimText, false);
    setupCaption (airFreqCaption, "FREQ", Theme::dimText, false);
    setupCaption (trimCaption, "TRIM", Theme::dimText, false);

    matchButton.setButtonText ("Match");
    matchButton.setColour (juce::TextButton::buttonColourId, Theme::trim.withAlpha (0.12f));
    matchButton.setColour (juce::TextButton::buttonOnColourId, Theme::trim.withAlpha (0.25f));
    matchButton.setColour (juce::TextButton::textColourOffId, Theme::text);
    matchButton.setColour (juce::TextButton::textColourOnId, Theme::text);
    matchButton.onClick = [this] { processorRef.requestGainMatch(); };
    addAndMakeVisible (matchButton);

    startTimerHz (10);
}

ControlPanel::~ControlPanel()
{
    for (auto* knob : { &presenceAmountKnob, &presenceFreqKnob, &airAmountKnob, &airFreqKnob, &trimKnob })
        knob->setLookAndFeel (nullptr);
}

//==============================================================================
void ControlPanel::setupKnob (juce::Slider& knob, juce::Colour accent)
{
    knob.setLookAndFeel (&knobLookAndFeel);
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 22);
    knob.setColour (juce::Slider::rotarySliderFillColourId, accent);
    knob.setColour (juce::Slider::rotarySliderOutlineColourId, Theme::knobOutline);
    knob.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.85f));
    knob.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    knob.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (knob);
}

void ControlPanel::setupCaption (juce::Label& label, const juce::String& text,
                                 juce::Colour colour, bool bold)
{
    const auto options = juce::FontOptions (11.0f);
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (bold ? options.withStyle ("Bold") : options));
    label.setColour (juce::Label::textColourId, colour.withMultipliedAlpha (bold ? 1.0f : 0.75f));
    addAndMakeVisible (label);
}

void ControlPanel::layoutKnobGroup (juce::Rectangle<int> area, juce::Label& title,
                                    juce::Slider& firstKnob, juce::Label& firstCaption,
                                    juce::Slider& secondKnob, juce::Label& secondCaption)
{
    title.setBounds (area.removeFromTop (20));

    auto firstArea = area.removeFromLeft (area.getWidth() / 2).reduced (6, 2);
    auto secondArea = area.reduced (6, 2);

    firstCaption.setBounds (firstArea.removeFromBottom (16));
    firstKnob.setBounds (firstArea);

    secondCaption.setBounds (secondArea.removeFromBottom (16));
    secondKnob.setBounds (secondArea);
}

//==============================================================================
void ControlPanel::paint (juce::Graphics& g)
{
    const auto bubble = getLocalBounds().reduced (shadowSize).toFloat();

    juce::Path bubblePath;
    bubblePath.addRoundedRectangle (bubble, cornerRadius);

    juce::DropShadow (juce::Colours::black.withAlpha (0.5f), shadowSize * 2, { 0, 3 })
        .drawForPath (g, bubblePath);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillPath (bubblePath);

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.strokePath (bubblePath, juce::PathStrokeType (1.0f));
}

void ControlPanel::resized()
{
    auto area = getLocalBounds().reduced (shadowSize).reduced (14, 10);

    const auto groupWidth = area.getWidth() / 3;

    layoutKnobGroup (area.removeFromLeft (groupWidth), presenceTitle, presenceAmountKnob,
                     presenceAmountCaption, presenceFreqKnob, presenceFreqCaption);
    layoutKnobGroup (area.removeFromLeft (groupWidth), airTitle, airAmountKnob,
                     airAmountCaption, airFreqKnob, airFreqCaption);

    outputTitle.setBounds (area.removeFromTop (20));

    auto trimArea = area.removeFromLeft (area.getWidth() / 2).reduced (6, 2);
    trimCaption.setBounds (trimArea.removeFromBottom (16));
    trimKnob.setBounds (trimArea);

    auto buttonArea = area.reduced (10, 4);
    matchButton.setBounds (buttonArea.withSizeKeepingCentre (juce::jmin (120, buttonArea.getWidth()), 32));
}

void ControlPanel::timerCallback()
{
    const auto matching = processorRef.isMatching();
    matchButton.setButtonText (matching ? "Matching..." : "Match");
    matchButton.setEnabled (! matching);
}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      spectrum (p),
      controlPanel (p)
{
    juce::ignoreUnused (processorRef);

    addAndMakeVisible (spectrum);
    addAndMakeVisible (controlPanel);

    setSize (680, 540);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (Theme::backgroundTop, 0.0f, 0.0f,
                                             Theme::backgroundBottom, 0.0f, (float) getHeight(), false));
    g.fillAll();
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    spectrum.setBounds (area);
    controlPanel.setBounds (area.reduced (8).removeFromBottom (ControlPanel::preferredHeight));
}
