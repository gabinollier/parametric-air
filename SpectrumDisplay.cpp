#include "SpectrumDisplay.h"

#include "Theme.h"

#include <cmath>

namespace
{
    constexpr std::array<float, 3> gridFrequencies { 100.0f, 1000.0f, 10000.0f };
    constexpr std::array<float, 3> gridDecibels { -60.0f, -30.0f, 0.0f };

    juce::String formatFrequency (float frequency)
    {
        return frequency >= 1000.0f ? juce::String (frequency / 1000.0f, 0) + " kHz"
                                    : juce::String (juce::roundToInt (frequency)) + " Hz";
    }
}

//==============================================================================
SpectrumDisplay::SpectrumDisplay (AudioPluginAudioProcessor& processor)
    : processorRef (processor)
{
    startTimerHz (30);
}

//==============================================================================
void SpectrumDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff101216));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    g.saveState();

    juce::Path clip;
    clip.addRoundedRectangle (bounds, 5.0f);
    g.reduceClipRegion (clip);

    drawGrid (g);
    drawLevels (g, inputLevels, Theme::spectrumInputLine, Theme::spectrumInputFill);
    drawLevels (g, presenceLevels, Theme::presence.withMultipliedSaturation (0.9f), Theme::presence);
    drawLevels (g, airLevels, Theme::air.withMultipliedSaturation (0.9f), Theme::air);

    g.restoreState();
}

//==============================================================================
void SpectrumDisplay::timerCallback()
{
    if (processorRef.getSampleRate() <= 0.0)
        return;

    auto needsRepaint = false;

    if (processorRef.getInputSpectrum().readLatest (inputSamples))
    {
        updateLevels (inputSamples, inputLevels);
        needsRepaint = true;
    }

    if (processorRef.getPresenceSpectrum().readLatest (presenceSamples))
    {
        updateLevels (presenceSamples, presenceLevels);
        needsRepaint = true;
    }

    if (processorRef.getAirSpectrum().readLatest (airSamples))
    {
        updateLevels (airSamples, airLevels);
        needsRepaint = true;
    }

    if (needsRepaint)
        repaint();
}

void SpectrumDisplay::updateLevels (const std::array<float, SpectrumBuffer::fftSize>& samples,
                                    std::array<float, numColumns>& levels)
{
    std::copy (samples.begin(), samples.end(), fftData.begin());
    std::fill (fftData.begin() + SpectrumBuffer::fftSize, fftData.end(), 0.0f);

    window.multiplyWithWindowingTable (fftData.data(), SpectrumBuffer::fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

    const auto sampleRate = processorRef.getSampleRate();
    const auto binWidth = (float) (sampleRate / SpectrumBuffer::fftSize);
    const auto numBins = SpectrumBuffer::fftSize / 2 + 1;

    for (int column = 0; column < numColumns; ++column)
    {
        const auto lowFrequency = frequencyForX ((float) column);
        const auto highFrequency = frequencyForX ((float) (column + 1));

        const auto lowBin = juce::jlimit (0, numBins - 1, (int) std::floor (lowFrequency / binWidth));
        const auto highBin = juce::jlimit (lowBin + 1, numBins, (int) std::ceil (highFrequency / binWidth));

        auto magnitude = 0.0f;

        for (int bin = lowBin; bin < highBin; ++bin)
            magnitude = juce::jmax (magnitude, fftData[(size_t) bin]);

        const auto amplitude = 2.0f * magnitude / (float) SpectrumBuffer::fftSize;
        const auto decibels = juce::jlimit (minDecibels, maxDecibels,
                                            juce::Decibels::gainToDecibels (amplitude, minDecibels));

        const auto decayed = levels[(size_t) column] - decayDbPerSecond / 30.0f;
        levels[(size_t) column] = juce::jmax (decibels, decayed);
    }
}

//==============================================================================
void SpectrumDisplay::drawGrid (juce::Graphics& g) const
{
    const auto font = juce::Font (juce::FontOptions (10.0f));
    g.setFont (font);

    for (auto frequency : gridFrequencies)
    {
        if (frequency < minFrequency || frequency > getDisplayMaxFrequency())
            continue;

        const auto x = juce::roundToInt (xForFrequency (frequency));
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawVerticalLine (x, 0.0f, (float) getHeight());

        g.setColour (juce::Colours::white.withAlpha (0.32f));
        g.drawText (formatFrequency (frequency), x + 4, 6, 60, 12,
                    juce::Justification::centredLeft);
    }

    for (auto decibels : gridDecibels)
    {
        const auto y = juce::roundToInt (yForDecibels (decibels));

        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawHorizontalLine (y, 0.0f, (float) getWidth());

        g.setColour (juce::Colours::white.withAlpha (0.32f));
        g.drawText (juce::String (juce::roundToInt (decibels)), 6, y - 13, 40, 12,
                    juce::Justification::centredLeft);
    }

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawText ("dB", 6, 6, 24, 12, juce::Justification::centredLeft);
}

void SpectrumDisplay::drawLevels (juce::Graphics& g, const std::array<float, numColumns>& levels,
                                  juce::Colour lineColour, juce::Colour fillColour) const
{
    const auto width = (float) getWidth();
    const auto height = (float) getHeight();
    const auto columnWidth = width / (float) numColumns;

    juce::Path curve;
    curve.startNewSubPath (0.0f, yForDecibels (levels[0]));

    for (int column = 1; column < numColumns; ++column)
        curve.lineTo ((float) column * columnWidth, yForDecibels (levels[(size_t) column]));

    curve.lineTo (width, yForDecibels (levels[numColumns - 1]));

    auto fill = curve;
    fill.lineTo (width, height);
    fill.lineTo (0.0f, height);
    fill.closeSubPath();

    g.setColour (fillColour.withAlpha (0.4f));
    g.fillPath (fill);

    g.setColour (lineColour.withAlpha (0.85f));
    g.strokePath (curve, juce::PathStrokeType (1.2f));
}

//==============================================================================
float SpectrumDisplay::getDisplayMaxFrequency() const
{
    return juce::jmin (maxFrequency, (float) processorRef.getSampleRate() * 0.45f);
}

float SpectrumDisplay::xForFrequency (float frequency) const
{
    return std::log (frequency / minFrequency) / std::log (getDisplayMaxFrequency() / minFrequency)
           * (float) getWidth();
}

float SpectrumDisplay::frequencyForX (float x) const
{
    return minFrequency * std::pow (getDisplayMaxFrequency() / minFrequency, x / (float) getWidth());
}

float SpectrumDisplay::yForDecibels (float decibels) const
{
    const auto clamped = juce::jlimit (minDecibels, maxDecibels, decibels);

    const auto fullSpanWeight = (maxDecibels - kneeDecibels)
                              + (kneeDecibels - minDecibels) * lowRangeScale;

    const auto weight = clamped >= kneeDecibels
                            ? maxDecibels - clamped
                            : (maxDecibels - kneeDecibels) + (kneeDecibels - clamped) * lowRangeScale;

    return weight / fullSpanWeight * (float) getHeight();
}
