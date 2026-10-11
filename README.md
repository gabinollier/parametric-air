# ParametricAir

ParametricAir is a presence and air enhancer. It's a two-band EQ where both bands are dynamic: each band is filtered out of the dry signal and boosted through a parallel path that gets compressed before being mixed back. Quiet parts get the full boost, loud parts get less of it, which is how it aims to give the openness of a big high-frequency lift without the harshness.

It's my first audio plugin, written in C++ with JUCE.

## How it works

Two fully sweepable bands:

- **Presence** (800 Hz to 6 kHz): definition and intelligibility.
- **Air** (4 kHz to 18 kHz): openness and shimmer.

Each band is split out of the dry signal with a band-pass filter, then run through a gain stage controlled by an envelope follower. The amount knob sets the maximum boost (up to 18 dB for presence, 20 dB for air), and the follower reduces it when the band gets loud. The band is added back on top of the dry signal, so the plugin processes in parallel.

The display on top shows the input spectrum along with the contribution of both bands in real time (input in blue, presence in orange, air in pink).

## Match

Boosting a band raises the output level, which makes "louder" easy to mistake for "better". The **Match** button measures the loudness of the input and the output over two seconds and sets the Trim so that both are equal. You can then judge the change in tone rather than the change in level.

The measurement uses K-weighting, the filter defined by ITU-R BS.1770 (the LUFS standard). The result is written to the Trim parameter, so it behaves like any other parameter in your DAW.

## Parameters

| Control         | Range           | Default |
| --------------- | --------------- | ------- |
| Presence Amount | 0 to 100%       | 0%      |
| Presence Freq   | 800 Hz to 6 kHz | 3 kHz   |
| Air Amount      | 0 to 100%       | 0%      |
| Air Freq        | 4 to 18 kHz     | 11 kHz  |
| Trim            | -18 to +18 dB   | 0 dB    |

## Building

Requires CMake 3.22 or newer and a C++17 compiler. JUCE is included as a Git submodule.

```sh
git clone --recurse-submodules https://github.com/gabinollier/ParametricAir.git
cd ParametricAir
cmake -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

If you cloned without the submodule, run `git submodule update --init` first.

On Linux, JUCE also needs a few development packages:

```sh
sudo apt install libasound2-dev libfreetype6-dev libgl1-mesa-dev libx11-dev \
    libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev
```

The build produces a VST3 plugin and a standalone application:

- `build/release/AudioPluginExample_artefacts/Release/VST3/ParametricAir.vst3`
- `build/release/AudioPluginExample_artefacts/Release/Standalone/ParametricAir`

On macOS it builds an AU as well. The CMake target is still named `AudioPluginExample` (a leftover from the JUCE template), which is why that name shows up in the build paths.

## Project structure

- `PluginProcessor.*`: audio processing, parameters and plugin state
- `DynamicBand.*`: the band-pass plus parallel compression stage, used twice
- `LoudnessMeter.*`: K-weighted loudness measurement for Match
- `PluginEditor.*`, `SpectrumDisplay.*`, `Theme.h`: the interface
- `SpectrumBuffer.h`: lock-free bridge between the audio and UI threads

Built with [JUCE](https://juce.com).
