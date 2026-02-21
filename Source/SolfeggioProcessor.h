#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

//==============================================================================
namespace Solfeggio {
    // Official Solfeggio frequencies + 432Hz (nature tuning)
    inline constexpr int NUM_FREQUENCIES = 10;
    
    inline constexpr std::array<float, NUM_FREQUENCIES> Frequencies = {
        174.0f, 285.0f, 396.0f, 417.0f, 432.0f,
        528.0f, 639.0f, 741.0f, 852.0f, 963.0f
    };

    inline const juce::StringArray FrequencyNames = {
        "174 Hz", "285 Hz", "396 Hz", "417 Hz", "432 Hz",
        "528 Hz", "639 Hz", "741 Hz", "852 Hz", "963 Hz"
    };

    inline const juce::StringArray FrequencyDescriptions = {
        "Pain Relief",       // 174 Hz
        "Tissue Healing",    // 285 Hz
        "Liberation",        // 396 Hz
        "Change",            // 417 Hz
        "Natural Tuning",    // 432 Hz
        "Transformation",    // 528 Hz
        "Connection",        // 639 Hz
        "Expression",        // 741 Hz
        "Intuition",         // 852 Hz
        "Enlightenment"      // 963 Hz
    };
}

//==============================================================================
class SidechainCompressor {
public:
    SidechainCompressor();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(float* data, const float* sidechainInput, int numSamples);
    void reset();

    // Thread-safe parameter setters
    void setAttackMs(float ms)   { attackMs.store(ms, std::memory_order_relaxed); }
    void setReleaseMs(float ms)  { releaseMs.store(ms, std::memory_order_relaxed); }
    void setThreshold(float dB)  { thresholdDb.store(dB, std::memory_order_relaxed); }
    void setRatio(float r)       { ratio.store(r, std::memory_order_relaxed); }
    void setKneeWidth(float dB)  { kneeWidthDb.store(dB, std::memory_order_relaxed); }
    void setDryWet(float w)      { dryWet.store(w, std::memory_order_relaxed); }

private:
    float computeGainReduction(float inputLevel) const;
    float computeEnvelopeRMS(const float* data, int numSamples);
    float computeEnvelopePeak(const float* data, int numSamples);

    // Atomic parameters (thread-safe)
    std::atomic<float> attackMs{10.0f};
    std::atomic<float> releaseMs{100.0f};
    std::atomic<float> thresholdDb{-20.0f};
    std::atomic<float> ratio{4.0f};
    std::atomic<float> kneeWidthDb{6.0f};
    std::atomic<float> dryWet{1.0f};

    // State
    double currentSampleRate = 44100.0;
    float envelopeLevel = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Sidechain bandpass filter
    juce::dsp::IIR::Filter<float> sidechainFilter;

    // RMS window
    std::vector<float> rmsBuffer;
    int rmsWritePos = 0;
    float rmsSum = 0.0f;
};

//==============================================================================
class SolfeggioProcessor : public juce::AudioProcessor {
public:
    SolfeggioProcessor();
    ~SolfeggioProcessor() override = default;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Program
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Presets
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    // State
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter tree
    juce::AudioProcessorValueTreeState apvts;

    // FFT data for spectrum analyzer (lock-free via atomic flag)
    static constexpr int fftOrder = 11;    // 2048 points
    static constexpr int fftSize = 1 << fftOrder;
    std::array<float, fftSize * 2> fftData{};
    std::atomic<bool> fftDataReady{false};

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static juce::AudioProcessor::BusesProperties getBusesProperties();

    // Oscillators for each solfeggio frequency
    struct SineOscillator {
        float phase = 0.0f;
        float phaseIncrement = 0.0f;

        void setFrequency(float freq, double sampleRate) {
            phaseIncrement = freq * 2.0f * juce::MathConstants<float>::pi
                           / static_cast<float>(sampleRate);
        }

        float next() {
            float sample = std::sin(phase);
            phase += phaseIncrement;
            if (phase > 2.0f * juce::MathConstants<float>::pi)
                phase -= 2.0f * juce::MathConstants<float>::pi;
            return sample;
        }
    };

    std::array<SineOscillator, Solfeggio::NUM_FREQUENCIES> oscillators;
    SidechainCompressor sidechain;

    // FFT for analysis
    juce::dsp::FFT forwardFFT{fftOrder};
    int fftFillIndex = 0;
    std::array<float, fftSize> fftInputBuffer{};

    // Smoothed gain values
    std::array<juce::SmoothedValue<float>, Solfeggio::NUM_FREQUENCIES> smoothedGains;
    juce::SmoothedValue<float> smoothedMix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SolfeggioProcessor)
};
