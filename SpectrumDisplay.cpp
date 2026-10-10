#include "SpectrumDisplay.h"

#include "Theme.h"

#include <cmath>

namespace
{
    constexpr std::array<float, 3> gridFrequencies { 100.0f, 1000.0f, 10000.0f };
    constexpr std::array<float, 5> gridDecibels { -80.0f, -60.0f, -40.0f, -20.0f, 0.0f };
    constexpr float maxBandCurveHeight = 0.5f;
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
    drawLevels (g, addedLevels, Theme::spectrumAddedLine, Theme::spectrumAddedFill);

    const auto presenceAmount = processorRef.parameters.getRawParameterValue ("presenceAmount")->load() * 0.01f;
    const auto airAmount = processorRef.parameters.getRawParameterValue ("airAmount")->load() * 0.01f;

    drawBandShape (g, processorRef.getPresenceBand(), presenceAmount, Theme::presence);
    drawBandShape (g, processorRef.getAirBand(), airAmount, Theme::air);

    g.restoreState();

    drawLegend (g);
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

    if (processorRef.getAddedSpectrum().readLatest (addedSamples))
    {
        updateLevels (addedSamples, addedLevels);
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
    g.setFont (juce::Font (juce::FontOptions (10.0f)));

    for (auto frequency : gridFrequencies)
    {
        if (frequency < minFrequency || frequency > getDisplayMaxFrequency())
            continue;

        const auto x = juce::roundToInt (xForFrequency (frequency));
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawVerticalLine (x, 0.0f, (float) getHeight());

        const auto text = frequency >= 1000.0f
                              ? juce::String (frequency / 1000.0f, 0) + "k"
                              : juce::String (frequency, 0);
        g.setColour (juce::Colours::white.withAlpha (0.28f));
        g.drawText (text, juce::Rectangle<int> (x + 3, getHeight() - 15, 40, 12),
                    juce::Justification::centredLeft);
    }

    for (auto decibels : gridDecibels)
    {
        if (decibels < minDecibels || decibels > maxDecibels)
            continue;

        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawHorizontalLine (juce::roundToInt (yForDecibels (decibels)), 0.0f, (float) getWidth());
    }
}

void SpectrumDisplay::drawLegend (juce::Graphics& g) const
{
    struct Entry
    {
        juce::Colour colour;
        const char* text;
    };

    const Entry entries[] {
        { Theme::spectrumInputLine, "IN" },
        { Theme::spectrumAddedLine, "PARALLEL" },
        { Theme::presence, "PRESENCE" },
        { Theme::air, "AIR" },
    };

    const auto font = juce::Font (juce::FontOptions (10.0f));
    g.setFont (font);

    auto x = 10;

    for (const auto& entry : entries)
    {
        g.setColour (entry.colour.withAlpha (0.9f));
        g.fillRoundedRectangle ((float) x, 11.0f, 8.0f, 8.0f, 2.0f);

        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawText (entry.text, x + 12, 8, 80, 12, juce::Justification::centredLeft);

        x += 12 + juce::GlyphArrangement::getStringWidthInt (font, entry.text) + 14;
    }
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

    g.setColour (fillColour.withAlpha (0.45f));
    g.fillPath (fill);

    g.setColour (lineColour.withAlpha (0.8f));
    g.strokePath (curve, juce::PathStrokeType (1.2f));
}

void SpectrumDisplay::drawBandShape (juce::Graphics& g, const DynamicBand& band,
                                     float amount01, juce::Colour colour) const
{
    const auto sampleRate = processorRef.getSampleRate();

    if (sampleRate <= 0.0)
        return;

    const auto width = (float) getWidth();
    const auto height = (float) getHeight();
    const auto visualAmount = juce::jmax (0.12f, amount01);
    const auto coefficients = band.getCurrentCoefficients();

    juce::Path curve;

    for (int x = 0; x <= juce::roundToInt (width); ++x)
    {
        const auto frequency = frequencyForX ((float) x);
        const auto magnitude = coefficients->getMagnitudeForFrequency ((double) frequency, sampleRate);
        const auto magnitudeDb = juce::jlimit (-24.0, 0.0, juce::Decibels::gainToDecibels (magnitude, -24.0));
        const auto shape = (float) (magnitudeDb + 24.0) / 24.0f;
        const auto y = height - shape * visualAmount * maxBandCurveHeight * height;

        if (x == 0)
            curve.startNewSubPath (0.0f, y);
        else
            curve.lineTo ((float) x, y);
    }

    auto fill = curve;
    fill.lineTo (width, height);
    fill.lineTo (0.0f, height);
    fill.closeSubPath();

    g.setColour (colour.withAlpha (0.10f + 0.15f * amount01));
    g.fillPath (fill);

    g.setColour (colour.withAlpha (0.45f + 0.35f * amount01));
    g.strokePath (curve, juce::PathStrokeType (1.4f));
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
    return (maxDecibels - decibels) / (maxDecibels - minDecibels) * (float) getHeight();
}
