// SolfeggioProcessor.h - Header limpo e moderno
#pragma once
#include <JuceHeader.h>
#include <array>
#include <memory>

namespace Solfeggio {
    // Frequências oficiais + 432Hz (natureza)
    inline constexpr std::array<float, 10> Frequencies = {
        174.0f, 285.0f, 396.0f, 417.0f, 432.0f, 
        528.0f, 639.0f, 741.0f, 852.0f, 963.0f
    };
    
    struct Channel {
        std::unique_ptr<juce::dsp::Oscillator<float>> oscillator;
        float gain = 0.0f;
        bool enabled = false;
    };
}

class SolfeggioProcessor : public juce::AudioProcessor {
public:
    SolfeggioProcessor();
    ~SolfeggioProcessor() override = default;

    // AudioProcessor overrides (implementação no .cpp)
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void releaseResources() override;
    
    // Parâmetros automáticos via AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState apvts;
    
private:
    std::array<Solfeggio::Channel, 10> channels;
    juce::dsp::Gain<float> masterGain;
    juce::dsp::Compressor<float> sidechainComp;
    
    // Análise espectral para mixagem dinâmica
    juce::dsp::FFT fft;
    std::vector<float> fftData;
    
    void updateOscillatorFrequencies();
    void applyDynamicMixing(juce::AudioBuffer<float>& buffer);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SolfeggioProcessor)
};