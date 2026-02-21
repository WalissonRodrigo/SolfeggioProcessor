#include "SolfeggioEngine.h"

SolfeggioEngine::SolfeggioEngine() {}

void SolfeggioEngine::prepare(double sampleRate, int samplesPerBlock) {
    for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
        auto idx = static_cast<size_t>(i);
        oscillators[idx].setFrequency(Solfeggio::Frequencies[idx], sampleRate);
        smoothedGains[idx].reset(sampleRate, 0.02);
        autoSmoothedGains[idx].reset(sampleRate, 0.05);
    }
    sidechain.prepare(sampleRate, samplesPerBlock);
    autoEngine.prepare(sampleRate);
    smoothedMix.reset(sampleRate, 0.02);
    fftFillIndex = 0;
    fftInputBuffer.fill(0.0f);
    fftData.fill(0.0f);
}

void SolfeggioEngine::reset() {
    sidechain.reset();
    autoEngine.reset();
}

void SolfeggioEngine::setSidechainParams(float attack, float release, float dryWet) {
    sidechain.setAttackMs(attack);
    sidechain.setReleaseMs(release);
    sidechain.setDryWet(dryWet);
}

void SolfeggioEngine::process(juce::AudioBuffer<float>& buffer,
                               bool autoMode,
                               float cycleTime,
                               float autoIntensity,
                               const std::array<float, Solfeggio::NUM_FREQUENCIES>& manualGains,
                               float masterMix)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    smoothedMix.setTargetValue(masterMix);

    std::array<float, Solfeggio::NUM_FREQUENCIES> targetGains {};
    if (autoMode) {
        autoEngine.analyzeBlock(buffer.getReadPointer(0), numSamples);
        autoEngine.getTargetGains(targetGains, cycleTime, autoIntensity);
        for (size_t i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i)
            autoSmoothedGains[i].setTargetValue(targetGains[i]);
    } else {
        for (size_t i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i)
            smoothedGains[i].setTargetValue(manualGains[i]);
    }

    for (int sample = 0; sample < numSamples; ++sample) {
        const float mix = smoothedMix.getNextValue();
        float solSample = 0.0f;

        for (size_t i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
            float gain = autoMode ? autoSmoothedGains[i].getNextValue()
                                  : smoothedGains[i].getNextValue();
            if (gain > 0.001f)
                solSample += oscillators[i].next() * gain * 0.1f;
        }

        for (int ch = 0; ch < numChannels; ++ch) {
            float* out   = buffer.getWritePointer(ch);
            float  music = out[sample];
            float  sideGain = juce::jlimit(0.3f, 1.0f, 1.0f - (std::abs(music) * 0.5f));
            out[sample] = music * (1.0f - mix * 0.3f) + solSample * mix * sideGain;
        }

        // Feed FFT
        fftInputBuffer[static_cast<size_t>(fftFillIndex)] = buffer.getSample(0, sample);
        if (++fftFillIndex >= fftSize) {
            fftFillIndex = 0;
            std::copy(fftInputBuffer.begin(), fftInputBuffer.end(), fftData.begin());
            std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
            forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());
            fftDataReady.store(true, std::memory_order_release);
        }
    }

    // Post-processing sidechain compression
    for (int ch = 0; ch < numChannels; ++ch)
        sidechain.process(buffer.getWritePointer(ch), buffer.getReadPointer(ch), numSamples);
}
