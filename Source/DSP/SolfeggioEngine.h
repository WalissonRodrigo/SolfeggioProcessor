#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include "Constants.h"
#include "SmartAutoEngine.h"
#include "SidechainCompressor.h"

// ============================================================================
// SolfeggioEngine
// Responsibility: Orchestrates all DSP — oscillators, FFT, auto engine, and
// sidechain compressor. This is the "Model" in the MVC/MVVM sense.
// ============================================================================
class SolfeggioEngine {
public:
    SolfeggioEngine();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer,
                 bool autoMode,
                 float cycleTime,
                 float autoIntensity,
                 const std::array<float, Solfeggio::NUM_FREQUENCIES>& manualGains,
                 float masterMix);
    void reset();

    void setSidechainParams(float attack, float release, float dryWet);

    // FFT access for SpectrumAnalyzer (read-only, lock-free)
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;
    const float* getFFTData() const { return fftData.data(); }
    bool isFFTDataReady() { return fftDataReady.exchange(false); }

    const SmartAutoEngine& getAutoEngine() const { return autoEngine; }

private:
    struct SineOscillator {
        float phase = 0.0f, phaseIncrement = 0.0f;
        void setFrequency(float freq, double sr) {
            phaseIncrement = freq * juce::MathConstants<float>::twoPi / static_cast<float>(sr);
        }
        float next() {
            float s = std::sin(phase);
            phase += phaseIncrement;
            if (phase > juce::MathConstants<float>::twoPi)
                phase -= juce::MathConstants<float>::twoPi;
            return s;
        }
    };

    std::array<SineOscillator, Solfeggio::NUM_FREQUENCIES>          oscillators;
    std::array<juce::SmoothedValue<float>, Solfeggio::NUM_FREQUENCIES> smoothedGains;
    std::array<juce::SmoothedValue<float>, Solfeggio::NUM_FREQUENCIES> autoSmoothedGains;
    juce::SmoothedValue<float> smoothedMix;

    SidechainCompressor sidechain;
    SmartAutoEngine     autoEngine;

    // FFT state
    juce::dsp::FFT forwardFFT { fftOrder };
    int fftFillIndex = 0;
    std::array<float, fftSize>     fftInputBuffer {};
    std::array<float, fftSize * 2> fftData        {};
    std::atomic<bool>              fftDataReady   { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SolfeggioEngine)
};
