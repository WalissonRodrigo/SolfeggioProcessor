# Contributing to Solfeggio Frequencies

Thank you for considering contributing! Here's how you can help.

## Getting Started

1. Fork the repository
2. Clone your fork and create a feature branch:
   ```bash
   git checkout -b feature/my-feature
   ```
3. Build the project (see [README.md](README.md#-building) for instructions)
4. Make your changes

## Code Style

- **C++20** standard
- Use JUCE coding conventions
- Header-only components where practical (e.g., `LookAndFeel.h`, `SpectrumAnalyzer.h`)
- Thread-safe parameter access via `std::atomic` or JUCE's `AudioProcessorValueTreeState`
- No raw `new`/`delete` — use `std::unique_ptr` and JUCE smart pointers

## Pull Request Process

1. Ensure the project builds on at least one platform (Linux, macOS, or Windows)
2. Test your changes with a DAW or the Standalone build
3. Update documentation if adding new features
4. Submit a PR against the `main` branch with a clear description

## Reporting Issues

Open an issue on GitHub with:
- Your OS and compiler version
- Steps to reproduce the problem
- Expected vs. actual behavior
- Relevant build logs (if applicable)

## Areas Where Help Is Welcome

- Unit tests (currently none — a test framework like Catch2 would be great)
- Additional Solfeggio frequency presets
- UI improvements and accessibility
- Linux packaging (AppImage, Flatpak)
- Documentation translations

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).
