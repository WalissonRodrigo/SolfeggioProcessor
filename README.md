# 🎵 Solfeggio Frequencies — Audio Plugin

[![Build](https://github.com/WalissonRodrigo/SolfeggioProcessor/actions/workflows/build.yml/badge.svg)](https://github.com/WalissonRodrigo/SolfeggioProcessor/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-gold.svg)](LICENSE)
![Version](https://img.shields.io/badge/version-1.0.0-purple)

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

The codebase follows a **layered MVC architecture** where each layer has a single responsibility:

```
Source/
├── Core/                         ← Shared constants, types, LookAndFeel
│   ├── Constants.h               # Parameter IDs, frequency metadata, layout
│   ├── LookAndFeel.h             # Dark theme (purple/gold palette)
│   └── WindowsIconHelpers.h      # Win32 taskbar icon helper
│
├── DSP/                          ← Model: all audio processing
│   ├── SolfeggioEngine.h/.cpp    # Orchestrator — oscillators, FFT, sidechain
│   ├── SmartAutoEngine.h/.cpp    # Spectral analysis & profile detection
│   └── SidechainCompressor.h/.cpp# Envelope follower + soft-knee compressor
│
├── GUI/                          ← View: visual components, zero DSP
│   ├── PluginEditor.h/.cpp       # Top-level layout container
│   ├── SpectrumAnalyzer.h        # Real-time FFT display
│   ├── AutoModeBar.h             # Auto-mode controls + profile label
│   ├── FrequencyGrid.h           # 10-knob frequency layout grid
│   └── FrequencyControl.h        # Individual frequency knob + toggle
│
└── Plugin/                       ← Controller: JUCE lifecycle & APVTS bridge
    ├── SolfeggioProcessor.h/.cpp  # Parameter layout, state I/O, DSP delegation
```

### Data Flow
```
User (APVTS params)  →  SolfeggioProcessor  →  SolfeggioEngine
                              ↓                      ↓
                         PluginEditor        SmartAutoEngine / SidechainCompressor
                              ↓
                        SpectrumAnalyzer ← FFT data (lock-free atomic)
```

### Smart Auto Mode — How It Works

1. **Spectral Analysis** — Splits input audio into bass/mid/high energy bands
2. **Profile Detection** — Classifies: Bass Heavy, Mid Focused, Bright, Full Spectrum, Quiet
3. **Frequency Selection** — Picks 3 Solfeggio frequencies matching the dominant spectrum
4. **Crossfade Cycling** — Smoothly transitions between sets every N seconds (15–120s)
5. **Adaptive Volume** — Reduces Solfeggio volume when music is loud for a subliminal effect

## 🔧 Building

### Prerequisites

- **CMake** ≥ 3.16
- **C++20** compatible compiler
- **JUCE 8.0.6** (fetched automatically via CMake `FetchContent`)

### Linux

```bash
sudo apt-get install -y libasound2-dev libjack-jackd2-dev libx11-dev \
    libxrandr-dev libxinerama-dev libxcursor-dev libfreetype-dev \
    libwebkit2gtk-4.1-dev libgtk-3-dev libcurl4-openssl-dev pkg-config

./scripts/build_linux.sh
```

### macOS (Apple Silicon)

```bash
./scripts/build_macos.sh
```

### Windows

```powershell
# PowerShell (recommended)
.\scripts\build_windows.ps1

# Or CMD
.\scripts\build_windows.bat
```

### Manual Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
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

GitHub Actions builds on every push to `main` / `develop` and on pull requests. Tagged releases (`v*`) trigger installer packaging (NSIS on Windows, DMG on macOS) and an automated **Public GitHub Release**.

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
