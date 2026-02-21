#include "SidechainCompressor.h"

SidechainCompressor::SidechainCompressor() {
    rmsBuffer.resize(256, 0.0f);
}

void SidechainCompressor::prepare(double sampleRate, int /*samplesPerBlock*/) {
    currentSampleRate = sampleRate;
    envelopeLevel = 0.0f;
    rmsWritePos = 0;
    rmsSum = 0.0f;
    std::fill(rmsBuffer.begin(), rmsBuffer.end(), 0.0f);

    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, 800.0f, 1.5f);
    sidechainFilter.coefficients = coeffs;
    sidechainFilter.reset();

    float atk = attackMs.load();
    float rel = releaseMs.load();
    attackCoeff  = 1.0f - std::exp(-1.0f / (static_cast<float>(sampleRate) * atk  * 0.001f));
    releaseCoeff = 1.0f - std::exp(-1.0f / (static_cast<float>(sampleRate) * rel * 0.001f));
}

void SidechainCompressor::reset() {
    envelopeLevel = 0.0f;
    rmsWritePos = 0;
    rmsSum = 0.0f;
    std::fill(rmsBuffer.begin(), rmsBuffer.end(), 0.0f);
    sidechainFilter.reset();
}

float SidechainCompressor::computeGainReduction(float levelDb) const {
    float thresh = thresholdDb.load();
    float r = ratio.load();
    float knee = kneeWidthDb.load();
    float halfKnee = knee * 0.5f;
    float diff = levelDb - thresh;
    if (diff <= -halfKnee) return 0.0f;
    if (diff >= halfKnee) return diff * (1.0f - 1.0f / r);
    float x = diff + halfKnee;
    return (x * x) / (4.0f * knee) * (1.0f - 1.0f / r);
}

void SidechainCompressor::process(float* data, const float* sidechainInput, int numSamples) {
    float atk = attackMs.load();
    float rel = releaseMs.load();
    float wet = dryWet.load();
    attackCoeff  = 1.0f - std::exp(-1.0f / (static_cast<float>(currentSampleRate) * atk  * 0.001f));
    releaseCoeff = 1.0f - std::exp(-1.0f / (static_cast<float>(currentSampleRate) * rel * 0.001f));

    for (int i = 0; i < numSamples; ++i) {
        float filtered = sidechainFilter.processSample(sidechainInput[i]);
        float level = std::abs(filtered);
        float coeff = (level > envelopeLevel) ? attackCoeff : releaseCoeff;
        envelopeLevel += coeff * (level - envelopeLevel);
        float levelDb = juce::Decibels::gainToDecibels(envelopeLevel, -100.0f);
        float reductionDb = computeGainReduction(levelDb);
        float gainLin = juce::Decibels::decibelsToGain(-reductionDb);
        data[i] = data[i] * (1.0f - wet) + (data[i] * gainLin) * wet;
    }
}
