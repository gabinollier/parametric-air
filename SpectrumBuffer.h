#pragma once

#include <juce_core/juce_core.h>

#include <algorithm>
#include <array>
#include <cstring>

class SpectrumBuffer
{
public:
    static constexpr int fftSize = 2048;

    void clear()
    {
        fifo.reset();
        ring.fill (0.0f);
        history.fill (0.0f);
    }

    void push (const float* samples, int numSamples)
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToWrite (numSamples, start1, size1, start2, size2);

        if (size1 > 0)
            std::copy (samples, samples + size1, ring.begin() + start1);

        if (size2 > 0)
            std::copy (samples + size1, samples + size1 + size2, ring.begin() + start2);

        fifo.finishedWrite (size1 + size2);
    }

    bool readLatest (std::array<float, fftSize>& dest)
    {
        const auto ready = fifo.getNumReady();

        if (ready <= 0)
            return false;

        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToRead (ready, start1, size1, start2, size2);

        appendToHistory (ring.data() + start1, size1);
        appendToHistory (ring.data() + start2, size2);
        fifo.finishedRead (size1 + size2);

        dest = history;
        return true;
    }

private:
    static constexpr int capacity = 16384;

    void appendToHistory (const float* data, int numSamples)
    {
        if (numSamples <= 0)
            return;

        if (numSamples >= fftSize)
        {
            std::copy (data + (numSamples - fftSize), data + numSamples, history.begin());
            return;
        }

        std::memmove (history.data(), history.data() + numSamples,
                      (size_t) (fftSize - numSamples) * sizeof (float));
        std::copy (data, data + numSamples, history.begin() + (fftSize - numSamples));
    }

    juce::AbstractFifo fifo { capacity };
    std::array<float, capacity> ring {};
    std::array<float, fftSize> history {};
};
