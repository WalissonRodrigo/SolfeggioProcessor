# 🎵 Solfeggio Frequencies — Audio Plugin

[![Build](https://github.com/WalissonRodrigo/SolfeggioProcessor/actions/workflows/build.yml/badge.svg)](https://github.com/WalissonRodrigo/SolfeggioProcessor/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-gold.svg)](LICENSE)
![Version](https://img.shields.io/badge/version-2.0.0-purple)

> Subliminal Solfeggio frequency processing plugin — blend healing tones into any audio, intelligently and imperceptibly.

---

## ✨ Features

| Feature | Description |
|---|---|
| **10 Solfeggio Frequencies** | 174 Hz (Pain Relief) through 963 Hz (Enlightenment), plus 432 Hz Natural Tuning |
| **Smart Auto Mode** | AI-driven frequency selection based on real-time spectral analysis of your music |
| **Sidechain Compressor** | Automatic ducking — Solfeggio tones hide beneath louder music passages |
| **Spectrum Analyzer** | Real-time FFT visualization with Solfeggio frequency markers |
| **Music Profile Detection** | Classifies audio as Bass Heavy, Mid Focused, Bright, Full Spectrum, or Quiet |
| **Crossfade Cycling** | Smooth 5-second crossfades when switching between active frequency sets |
| **Custom UI** | Dark theme with purple/gold accents, custom rotary knobs, and animated controls |
| **Cross-Platform** | Builds on Linux, macOS (ARM64), and Windows |

## 🎛️ Plugin Formats

- **VST3** — Compatible with all major DAWs (Ableton, FL Studio, Reaper, etc.)
- **Standalone** — Run independently without a DAW
- **AU** — Audio Unit for macOS (Logic Pro, GarageBand)

## 🏗️ Architecture

```
Source/
├── SolfeggioProcessor.h/.cpp   # Audio processor, Smart Auto Engine, Sidechain Compressor
├── PluginEditor.h/.cpp         # GUI with frequency knobs, auto controls, spectrum display
├── LookAndFeel.h               # Custom dark theme (purple/gold palette)
└── SpectrumAnalyzer.h          # Real-time FFT spectrum visualizer
```

### Core Components

- **`SolfeggioProcessor`** — Main audio processor with 10 sine oscillators, parameter management, FFT analysis, and state serialization
- **`SmartAutoEngine`** — Analyzes audio into bass/mid/high energy bands, detects music profile, selects and crossfades Solfeggio frequencies
- **`SidechainCompressor`** — RMS/peak envelope follower with soft-knee compression for intelligent ducking
- **`SpectrumAnalyzer`** — Logarithmic FFT display (20 Hz–20 kHz) with Solfeggio frequency markers
- **`SolfeggioLookAndFeel`** — Custom JUCE LookAndFeel_V4 with gradient rotary sliders, toggle buttons, and linear sliders

### Smart Auto Mode — How It Works

1. **Spectral Analysis** — Splits input audio into bass (20–300 Hz), mid (300–2 kHz), and high (2–10 kHz) bands
2. **Profile Detection** — Classifies the energy distribution (e.g., "Bass Heavy" for EDM, "Mid Focused" for vocals)
3. **Frequency Selection** — Picks 3 Solfeggio frequencies that hide best within the dominant spectral bands
4. **Crossfade Cycling** — Every N seconds (configurable 15–120s), smoothly transitions to a new set of frequencies
5. **Adaptive Volume** — Automatically reduces Solfeggio volume when music is loud for a truly subliminal effect

## 🔧 Building

### Prerequisites

- **CMake** ≥ 3.16
- **C++20** compatible compiler
- **JUCE 8.0.6** (fetched automatically via CMake `FetchContent`)

### Linux

```bash
sudo apt-get install -y libasound2-dev libjack-jackd2-dev libx11-dev \
    libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev \
    libwebkit2gtk-4.1-dev libgtk-3-dev libcurl4-openssl-dev pkg-config

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

### macOS (Apple Silicon)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --config Release --parallel
```

### Windows

Use the provided build scripts:

```powershell
# PowerShell (recommended)
.\build_windows.ps1

# Or CMD
.\build_windows.bat
```

Or manually:

```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

### Build Output

| Artifact | Location |
|---|---|
| VST3 Plugin | `build/**/VST3/*.vst3` |
| Standalone App | `build/**/Standalone/` |
| AU Plugin (macOS) | `build/**/AU/*.component` |

## 📦 Installation

### VST3

Copy the `.vst3` folder to your system's VST3 directory:

| OS | Path |
|---|---|
| Windows | `C:\Program Files\Common Files\VST3\` |
| macOS | `~/Library/Audio/Plug-Ins/VST3/` |
| Linux | `~/.vst3/` |

### Standalone

Run the executable directly from the build output.

## 🔄 CI/CD

GitHub Actions builds on every push to `main` / `develop` and on pull requests. Tagged releases (`v*`) trigger installer packaging (NSIS on Windows, DMG on macOS) and a draft GitHub Release.

## 🎶 Solfeggio Frequencies Reference

| Frequency | Purpose |
|---|---|
| 174 Hz | Pain Relief |
| 285 Hz | Tissue Healing |
| 396 Hz | Liberation |
| 417 Hz | Change |
| 432 Hz | Natural Tuning |
| 528 Hz | Transformation |
| 639 Hz | Connection |
| 741 Hz | Expression |
| 852 Hz | Intuition |
| 963 Hz | Enlightenment |

## 📄 License

[MIT](LICENSE) — © 2024–2026 Walisson Rodrigo

## 🤝 Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.
