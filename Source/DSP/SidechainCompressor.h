#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include <cmath>

// ============================================================================
// SidechainCompressor
// Responsibility: RMS envelope tracking + soft-knee compression for ducking.
// ============================================================================
class SidechainCompressor {
public:
    SidechainCompressor();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // Process data in-place, using sidechainInput as the key signal.
    void process(float* data, const float* sidechainInput, int numSamples);

    // Thread-safe parameter setters
    void setAttackMs(float ms)    { attackMs.store(ms); }
    void setReleaseMs(float ms)   { releaseMs.store(ms); }
    void setDryWet(float w)       { dryWet.store(w); }
    void setThresholdDb(float db) { thresholdDb.store(db); }
    void setRatio(float r)        { ratio.store(r); }

private:
    float computeGainReduction(float levelDb) const;

    double currentSampleRate = 44100.0;
    float envelopeLevel = 0.0f;
    float attackCoeff = 0.01f, releaseCoeff = 0.001f;

    std::atomic<float> attackMs    { 10.0f };
    std::atomic<float> releaseMs   { 100.0f };
    std::atomic<float> dryWet      { 0.5f };
    std::atomic<float> thresholdDb { -18.0f };
    std::atomic<float> ratio       { 4.0f };
    std::atomic<float> kneeWidthDb { 6.0f };

    juce::dsp::IIR::Filter<float> sidechainFilter;

    std::vector<float> rmsBuffer;
    int rmsWritePos = 0;
    float rmsSum = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidechainCompressor)
};
