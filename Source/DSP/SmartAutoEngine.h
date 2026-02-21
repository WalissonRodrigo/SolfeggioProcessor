#pragma once
#include <JuceHeader.h>
#include <array>
#include "Constants.h"

// ============================================================================
// SmartAutoEngine
// Responsibility: Real-time spectral energy analysis → profile detection →
// intelligent Solfeggio frequency selection with smooth crossfading.
// ============================================================================
class SmartAutoEngine {
public:
    SmartAutoEngine();

    void prepare(double sampleRate);
    void reset();

    // Feed audio samples for spectral analysis
    void analyzeBlock(const float* data, int numSamples);

    // Fill `gains` with target values for each frequency (0..1)
    void getTargetGains(std::array<float, Solfeggio::NUM_FREQUENCIES>& gains,
                        float cycleTimeSec, float intensity);

    enum class MusicProfile { BassHeavy, MidFocused, Bright, FullSpectrum, Quiet };

    MusicProfile getCurrentProfile() const { return currentProfile; }

private:
    double sampleRate = 44100.0;

    juce::dsp::IIR::Filter<float> bassFilter, midFilter, highFilter;
    float smoothBass = 0.0f, smoothMid = 0.0f, smoothHigh = 0.0f, smoothTotal = 0.0f;
    static constexpr float smoothCoeff = 0.95f;

    double cycleTimer = 0.0;
    int currentCycleSlot = 0;
    static constexpr int NUM_CYCLE_SLOTS = 3;

    std::array<int, NUM_CYCLE_SLOTS> activeFreqs = { 0, 5, 9 };
    std::array<int, NUM_CYCLE_SLOTS> nextFreqs   = { 1, 6, 8 };

    float crossfadeProgress = 1.0f;
    static constexpr float crossfadeDurationSec = 5.0f;
    bool isCrossfading = false;

    MusicProfile currentProfile = MusicProfile::Quiet;

    MusicProfile detectProfile() const;
    void selectFrequenciesForProfile(MusicProfile profile,
                                     std::array<int, NUM_CYCLE_SLOTS>& selection);
};
