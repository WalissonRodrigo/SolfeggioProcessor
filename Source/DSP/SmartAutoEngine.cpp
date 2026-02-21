#include "SmartAutoEngine.h"

SmartAutoEngine::SmartAutoEngine() {}

void SmartAutoEngine::prepare(double sr) {
    sampleRate = sr;
    bassFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 300.0f);
    midFilter.coefficients  = juce::dsp::IIR::Coefficients<float>::makeBandPass(sr, 800.0f, 0.8f);
    highFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 2000.0f);
    reset();
}

void SmartAutoEngine::reset() {
    smoothBass = smoothMid = smoothHigh = smoothTotal = 0.0f;
    cycleTimer = 0.0;
    crossfadeProgress = 1.0f;
    isCrossfading = false;
    currentProfile = MusicProfile::Quiet;
}

SmartAutoEngine::MusicProfile SmartAutoEngine::detectProfile() const {
    float total = smoothBass + smoothMid + smoothHigh + 1e-10f;
    if (total < 0.001f)           return MusicProfile::Quiet;
    if (smoothBass  / total > 0.5f)  return MusicProfile::BassHeavy;
    if (smoothMid   / total > 0.45f) return MusicProfile::MidFocused;
    if (smoothHigh  / total > 0.4f)  return MusicProfile::Bright;
    return MusicProfile::FullSpectrum;
}

void SmartAutoEngine::selectFrequenciesForProfile(MusicProfile profile,
                                                   std::array<int, NUM_CYCLE_SLOTS>& sel) {
    switch (profile) {
        case MusicProfile::BassHeavy:    sel = { 5, 6, 7 }; break;
        case MusicProfile::MidFocused:   sel = { 3, 4, 5 }; break;
        case MusicProfile::Bright:       sel = { 7, 8, 9 }; break;
        case MusicProfile::FullSpectrum: sel = { 2, 5, 8 }; break;
        case MusicProfile::Quiet:        sel = { 0, 1, 2 }; break;
    }
    int offset = currentCycleSlot % 3;
    if (offset > 0)
        for (auto& idx : sel) idx = (idx + offset) % Solfeggio::NUM_FREQUENCIES;
}

void SmartAutoEngine::analyzeBlock(const float* data, int numSamples) {
    float b = 0.0f, m = 0.0f, h = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        b += std::pow(bassFilter.processSample(data[i]), 2.0f);
        m += std::pow(midFilter.processSample(data[i]),  2.0f);
        h += std::pow(highFilter.processSample(data[i]), 2.0f);
    }
    float invN = 1.0f / static_cast<float>(std::max(numSamples, 1));
    smoothBass  = smoothCoeff * smoothBass  + (1.0f - smoothCoeff) * std::sqrt(b * invN);
    smoothMid   = smoothCoeff * smoothMid   + (1.0f - smoothCoeff) * std::sqrt(m * invN);
    smoothHigh  = smoothCoeff * smoothHigh  + (1.0f - smoothCoeff) * std::sqrt(h * invN);
    smoothTotal = smoothBass + smoothMid + smoothHigh;
    currentProfile = detectProfile();
}

void SmartAutoEngine::getTargetGains(std::array<float, Solfeggio::NUM_FREQUENCIES>& gains,
                                      float cycleTimeSec, float intensity) {
    constexpr double blockDur = 1.0 / 60.0;
    cycleTimer += blockDur;

    if (isCrossfading) {
        crossfadeProgress += static_cast<float>(blockDur / crossfadeDurationSec);
        if (crossfadeProgress >= 1.0f) {
            crossfadeProgress = 1.0f;
            isCrossfading = false;
            activeFreqs = nextFreqs;
        }
    }

    if (cycleTimer >= static_cast<double>(cycleTimeSec) && !isCrossfading) {
        cycleTimer = 0.0;
        ++currentCycleSlot;
        selectFrequenciesForProfile(currentProfile, nextFreqs);
        if (nextFreqs != activeFreqs) { isCrossfading = true; crossfadeProgress = 0.0f; }
    }

    gains.fill(0.0f);
    float activeG = isCrossfading ? (1.0f - crossfadeProgress) : 1.0f;
    for (int i : activeFreqs) gains[static_cast<size_t>(i)] += activeG;
    if (isCrossfading)
        for (int i : nextFreqs)  gains[static_cast<size_t>(i)] += crossfadeProgress;

    float adaptiveVol = smoothTotal > 0.01f
        ? juce::jlimit(0.2f, 1.0f, 0.5f / (smoothTotal + 0.5f)) : 1.0f;

    for (auto& g : gains)
        g = juce::jlimit(0.0f, 1.0f, g * intensity * adaptiveVol);
}
