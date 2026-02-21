// SolfeggioProcessor.cpp - Implementação com abstrações modernas
#include "SolfeggioProcessor.h"

SolfeggioProcessor::SolfeggioProcessor()
    : AudioProcessor(getBusesProperties()),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      fft(11) // 2048 pontos
{
    // Inicializa osciladores senoidais puros
    for (size_t i = 0; i < Solfeggio::Frequencies.size(); ++i) {
        auto& ch = channels[i];
        ch.oscillator = std::make_unique<juce::dsp::Oscillator<float>>(
            [](float x) { return std::sin(x); } // Onda seno pura
        );
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout 
SolfeggioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Cria parâmetros automatizáveis para cada frequência
    for (size_t i = 0; i < Solfeggio::Frequencies.size(); ++i) {
        auto freq = Solfeggio::Frequencies[i];
        auto name = juce::String(freq) + "Hz_Gain";
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            name, name,
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
            0.0f,
            juce::AudioParameterFloatAttributes()
                .withLabel(juce::String(freq) + " Hz")
        ));
        
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            name + "_Enabled", name + " On", false
        ));
    }
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "MasterMix", "Mix %",
        juce::NormalisableRange<float>(0.0f, 100.0f), 15.0f
    ));
    
    return { params.begin(), params.end() };
}

void SolfeggioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::dsp::ProcessSpec spec{
        sampleRate, 
        static_cast<juce::uint32>(samplesPerBlock),
        static_cast<juce::uint32>(getTotalNumInputChannels())
    };
    
    // Prepara todos os processadores
    masterGain.prepare(spec);
    sidechainComp.prepare(spec);
    
    for (auto& ch : channels) {
        ch.oscillator->prepare(spec);
    }
    
    fftData.resize(fft.getSize() * 2);
    updateOscillatorFrequencies();
}

void SolfeggioProcessor::updateOscillatorFrequencies() {
    auto sr = getSampleRate();
    for (size_t i = 0; i < channels.size(); ++i) {
        channels[i].oscillator->setFrequency(Solfeggio::Frequencies[i]);
    }
}

void SolfeggioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, 
    juce::MidiBuffer&
) {
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // Copia input para output se necessário
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    
    // Análise espectral para mixagem dinâmica
    applyDynamicMixing(buffer);
    
    // Gera e mixa frequências Solfeggio
    juce::AudioBuffer<float> solfeggioBuffer(
        totalNumOutputChannels, 
        buffer.getNumSamples()
    );
    solfeggioBuffer.clear();
    
    for (size_t i = 0; i < channels.size(); ++i) {
        auto& ch = channels[i];
        auto gainParam = apvts.getRawParameterValue(
            juce::String(Solfeggio::Frequencies[i]) + "Hz_Gain"
        );
        auto enabledParam = apvts.getRawParameterValue(
            juce::String(Solfeggio::Frequencies[i]) + "Hz_Gain_Enabled"
        );
        
        if (!enabledParam->load() || gainParam->load() <= 0.0f) continue;
        
        ch.gain = gainParam->load();
        
        // Processa oscilador canal por canal
        for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
            auto* channelData = solfeggioBuffer.getWritePointer(channel);
            
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                // Gera amostra senoidal
                float oscSample = ch.oscillator->processSample(0.0f);
                channelData[sample] += oscSample * ch.gain * 0.1f; // -20dB subliminar
            }
        }
    }
    
    // Mixagem final com sidechain
    auto mix = apvts.getRawParameterValue("MasterMix")->load() / 100.0f;
    
    for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
        auto* outputData = buffer.getWritePointer(channel);
        auto* solfeggioData = solfeggioBuffer.getReadPointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            // Sidechain: reduz Solfeggio quando música está alta
            float sidechainGain = 1.0f - (std::abs(outputData[sample]) * 0.5f);
            sidechainGain = juce::jlimit(0.3f, 1.0f, sidechainGain);
            
            outputData[sample] = outputData[sample] * (1.0f - mix * 0.3f) 
                               + solfeggioData[sample] * mix * sidechainGain;
        }
    }
}

void SolfeggioProcessor::applyDynamicMixing(juce::AudioBuffer<float>& buffer) {
    // FFT para análise espectral (simplificado)
    if (buffer.getNumChannels() > 0) {
        auto* data = buffer.getReadPointer(0);
        std::copy(data, data + std::min(buffer.getNumSamples(), (int)fftData.size()), 
                  fftData.begin());
        fft.performFrequencyOnlyForwardTransform(fftData.data());
    }
}

// Exporta factory function
extern "C" {
    juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
        return new SolfeggioProcessor();
    }
}