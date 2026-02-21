# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-02-21

### Added
- **Smart Auto Mode** — AI-driven frequency selection based on real-time spectral analysis
  - Music profile detection (Bass Heavy, Mid Focused, Bright, Full Spectrum, Quiet)
  - Automatic frequency cycling with configurable period (15–120 seconds)
  - Smooth 5-second crossfades between frequency sets
  - Adaptive volume control for subliminal effect
  - Configurable intensity (0–100%)
- **Sidechain Compressor** — RMS/peak envelope follower with soft-knee compression
  - Configurable attack, release, and dry/wet controls
  - Bandpass-filtered sidechain input (200 Hz–2 kHz)
- **Spectrum Analyzer** — Real-time FFT visualizer (2048-point)
  - Logarithmic frequency scale (20 Hz–20 kHz)
  - Solfeggio frequency markers with gold annotations
  - Gradient fill under the spectrum curve
- **Custom Look & Feel** — Dark theme with purple/gold accents
  - Custom rotary sliders with gradient arcs and glow effects
  - Custom toggle buttons with animated state indicators
  - Custom linear slider with gradient fill and thumb glow
- **432 Hz Natural Tuning** frequency added to the Solfeggio set
- **Smart Auto GUI controls** — AUTO toggle, cycle time slider, intensity slider, profile display
- **macOS ARM64 support** in CI/CD pipeline
- **macOS AU (Audio Unit)** format output
- **CPack installers** — NSIS for Windows, DMG for macOS
- **GitHub Actions CI/CD** with automated builds for Linux, macOS, and Windows
- **GitHub Release automation** for tagged versions
- Project documentation (`README.md`, `CHANGELOG.md`, `CONTRIBUTING.md`, `LICENSE`)

### Changed
- Architecture-aware SIMD flags (x86 AVX2/SSE vs. ARM NEON auto-detect)
- Improved parameter system using `juce::ParameterID` with version tracking

### Removed
- Orphaned Rust/nih-plug prototype code (`Cargo.toml`, `src/SolfeggioNIH.rs`)
- Orphaned iPlug2 prototype header (`SolfeggioIPlug.h`)
- Legacy prototype files at project root (`SolfeggioProcessor.h`, `main.h`)
- Debugging log files

## [1.0.0] - 2026-02-20

### Added
- Initial Solfeggio Frequencies plugin implementation
- 9 Solfeggio frequencies (174–963 Hz) with individual gain and enable controls
- Master mix control (0–100%)
- Basic sidechain ducking (level-based)
- VST3 and Standalone output formats
- JUCE 8.0.6 via CMake FetchContent
- Cross-platform CMake build system
- Windows build scripts (`build_windows.bat`, `build_windows.ps1`)
