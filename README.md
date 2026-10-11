# ParametricAir

Language: [[Fr](README-fr.md) | En]

!["Screenshots of the plugin"](README-assets/screenshot.png)

ParametricAir is an audio plugin meant to enhance presence and air, inspired by FreshAir. 
It's my first audio plugin, written in C++ with JUCE. It's more of a learning project than a finished project, but it's definitely usable.

## How it works

The input signal is split into two bands: **presence** (800 Hz to 6 kHz) and **air** (4 kHz to 18 kHz).
Each band gets compressed in parallel before being mixed back in the input signal. 

Quiet parts get the full boost, loud parts get less of it, which is how it aims to give the openness of a big high-frequency lift without the harshness.

The display on top shows the input spectrum along with the contribution of both bands in real time (input in gray, presence in yellow/orange, air in pink).

## Building

Requires CMake 3.22 or newer and a C++17 compiler. JUCE is included as a Git submodule.

```sh
git clone --recurse-submodules https://github.com/gabinollier/ParametricAir.git
cd ParametricAir
cmake -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

If you cloned without the submodule, run `git submodule update --init` first.

On Linux, JUCE usually also needs a few packages:

```sh
sudo apt install libasound2-dev libfreetype6-dev libgl1-mesa-dev libx11-dev \
    libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev
```

The build produces a VST3 plugin and a standalone application (mainly for testing).