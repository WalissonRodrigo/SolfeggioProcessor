#include "SolfeggioProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// SidechainCompressor Implementation
//==============================================================================

SidechainCompressor::SidechainCompressor() {
    rmsBuffer.resize(256, 0.0f);
}

void SidechainCompressor::prepare(double sampleRate, int /*samplesPerBlock*/) {
    currentSampleRate = sampleRate;
    envelopeLevel = 0.0f;
    rmsWritePos = 0;
    rmsSum = 0.0f;
    std::fill(rmsBuffer.begin(), rmsBuffer.end(), 0.0f);

    // Setup sidechain bandpass filter (200Hz - 2kHz for voice/music detection)
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
        sampleRate, 800.0f, 1.5f);
    sidechainFilter.coefficients = coeffs;
    sidechainFilter.reset();

    // Pre-compute coefficients
    float atk = attackMs.load(std::memory_order_relaxed);
    float rel = releaseMs.load(std::memory_order_relaxed);
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

float SidechainCompressor::computeGainReduction(float inputLevelDb) const {
    float thresh = thresholdDb.load(std::memory_order_relaxed);
    float r = ratio.load(std::memory_order_relaxed);
    float knee = kneeWidthDb.load(std::memory_order_relaxed);

    float halfKnee = knee * 0.5f;
    float diff = inputLevelDb - thresh;

    if (diff <= -halfKnee) {
        // Below knee → no compression
        return 0.0f;
    } else if (diff >= halfKnee) {
        // Above knee → full compression
        return diff * (1.0f - 1.0f / r);
    } else {
        // In the knee → soft transition
        float x = diff + halfKnee;
        return (x * x) / (4.0f * knee) * (1.0f - 1.0f / r);
    }
}

float SidechainCompressor::computeEnvelopeRMS(const float* data, int numSamples) {
    // Use JUCE's optimized vector operations (SIMD when available)
    float sumOfSquares = 0.0f;

    // Process 4 samples at a time using FloatVectorOperations
    for (int i = 0; i < numSamples; ++i) {
        float filtered = sidechainFilter.processSample(data[i]);
        float sq = filtered * filtered;

        // Update running RMS window
        rmsSum -= rmsBuffer[static_cast<size_t>(rmsWritePos)];
        rmsBuffer[static_cast<size_t>(rmsWritePos)] = sq;
        rmsSum += sq;
        rmsWritePos = (rmsWritePos + 1) % static_cast<int>(rmsBuffer.size());

        sumOfSquares += sq;
    }

    return std::sqrt(rmsSum / static_cast<float>(rmsBuffer.size()));
}

float SidechainCompressor::computeEnvelopePeak(const float* data, int numSamples) {
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        float filtered = std::abs(data[i]);
        peak = std::max(peak, filtered);
    }
    return peak;
}

void SidechainCompressor::process(float* data, const float* sidechainInput, int numSamples) {
    float atk = attackMs.load(std::memory_order_relaxed);
    float rel = releaseMs.load(std::memory_order_relaxed);
    float wet = dryWet.load(std::memory_order_relaxed);

    // Recompute coefficients if changed
    attackCoeff  = 1.0f - std::exp(-1.0f / (static_cast<float>(currentSampleRate) * atk  * 0.001f));
    releaseCoeff = 1.0f - std::exp(-1.0f / (static_cast<float>(currentSampleRate) * rel * 0.001f));

    for (int i = 0; i < numSamples; ++i) {
        // Envelope follower: peak + RMS hybrid
        float filtered = sidechainFilter.processSample(sidechainInput[i]);
        float inputLevel = std::abs(filtered);

        // Smooth envelope with attack/release
        float coeff = (inputLevel > envelopeLevel) ? attackCoeff : releaseCoeff;
        envelopeLevel += coeff * (inputLevel - envelopeLevel);

        // Convert to dB
        float levelDb = juce::Decibels::gainToDecibels(envelopeLevel, -100.0f);

        // Compute gain reduction
        float reductionDb = computeGainReduction(levelDb);
        float gainLin = juce::Decibels::decibelsToGain(-reductionDb);

        // Apply with dry/wet
        float compressed = data[i] * gainLin;
        data[i] = data[i] * (1.0f - wet) + compressed * wet;
    }
}

//==============================================================================
// SolfeggioProcessor Implementation
//==============================================================================

SolfeggioProcessor::SolfeggioProcessor()
    : AudioProcessor(getBusesProperties()),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessor::BusesProperties SolfeggioProcessor::getBusesProperties() {
    return BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true);
}

juce::AudioProcessorValueTreeState::ParameterLayout
SolfeggioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
        auto freqStr = juce::String(Solfeggio::Frequencies[static_cast<size_t>(i)], 0);

        // Gain parameter for each frequency
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{freqStr + "Hz_Gain", 1},
            freqStr + " Hz Gain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
            0.0f));

        // Enable toggle for each frequency
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{freqStr + "Hz_On", 1},
            freqStr + " Hz On",
            false));
    }

    // Master mix
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"MasterMix", 1},
        "Master Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        15.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + "%"; },
        [](const juce::String& s) { return s.getFloatValue(); }));

    // Sidechain parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"SC_Attack", 1}, "SC Attack",
        juce::NormalisableRange<float>(1.0f, 1000.0f, 0.1f, 0.3f), 10.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"SC_Release", 1}, "SC Release",
        juce::NormalisableRange<float>(1.0f, 1000.0f, 0.1f, 0.3f), 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"SC_DryWet", 1}, "SC Dry/Wet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    return {params.begin(), params.end()};
}

void SolfeggioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Setup oscillators
    for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
        oscillators[static_cast<size_t>(i)].setFrequency(
            Solfeggio::Frequencies[static_cast<size_t>(i)], sampleRate);
    }

    // Setup sidechain
    sidechain.prepare(sampleRate, samplesPerBlock);

    // Setup smoothed values
    for (auto& sg : smoothedGains) {
        sg.reset(sampleRate, 0.02); // 20ms smoothing
    }
    smoothedMix.reset(sampleRate, 0.02);

    // Reset FFT
    fftFillIndex = 0;
    fftInputBuffer.fill(0.0f);
    fftData.fill(0.0f);
}

void SolfeggioProcessor::releaseResources() {
    sidechain.reset();
}

bool SolfeggioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void SolfeggioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Read parameters
    auto mixParam = apvts.getRawParameterValue("MasterMix");
    float mix = mixParam->load() / 100.0f;
    smoothedMix.setTargetValue(mix);

    auto scAttack  = apvts.getRawParameterValue("SC_Attack")->load();
    auto scRelease = apvts.getRawParameterValue("SC_Release")->load();
    auto scDryWet  = apvts.getRawParameterValue("SC_DryWet")->load();

    sidechain.setAttackMs(scAttack);
    sidechain.setReleaseMs(scRelease);
    sidechain.setDryWet(scDryWet);

    // Read frequency gains and enabled states
    std::array<float, Solfeggio::NUM_FREQUENCIES> gains{};
    std::array<bool,  Solfeggio::NUM_FREQUENCIES> enabled{};

    for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
        auto freqStr = juce::String(Solfeggio::Frequencies[static_cast<size_t>(i)], 0);
        float g = apvts.getRawParameterValue(freqStr + "Hz_Gain")->load();
        bool  e = apvts.getRawParameterValue(freqStr + "Hz_On")->load() > 0.5f;
        gains[static_cast<size_t>(i)] = e ? g : 0.0f;
        enabled[static_cast<size_t>(i)] = e;
        smoothedGains[static_cast<size_t>(i)].setTargetValue(gains[static_cast<size_t>(i)]);
    }

    // Generate solfeggio signal
    juce::AudioBuffer<float> solfeggioBuffer(totalNumOutputChannels, numSamples);
    solfeggioBuffer.clear();

    for (int sample = 0; sample < numSamples; ++sample) {
        float currentMix = smoothedMix.getNextValue();

        // Accumulate all active oscillators
        float solfeggioSample = 0.0f;
        for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
            float gain = smoothedGains[static_cast<size_t>(i)].getNextValue();
            if (gain > 0.001f) {
                solfeggioSample += oscillators[static_cast<size_t>(i)].next()
                                 * gain * 0.1f; // -20dB subliminal level
            }
        }

        // Apply to each channel with sidechain ducking
        for (int ch = 0; ch < totalNumOutputChannels; ++ch) {
            float* outputData = buffer.getWritePointer(ch);

            // Dynamic sidechain: reduce solfeggio when music is loud
            float inputLevel = std::abs(outputData[sample]);
            float sidechainGain = 1.0f - (inputLevel * 0.5f);
            sidechainGain = juce::jlimit(0.3f, 1.0f, sidechainGain);

            outputData[sample] = outputData[sample] * (1.0f - currentMix * 0.3f)
                               + solfeggioSample * currentMix * sidechainGain;
        }

        // Feed FFT buffer (channel 0 only)
        fftInputBuffer[static_cast<size_t>(fftFillIndex)] = buffer.getSample(0, sample);
        if (++fftFillIndex >= fftSize) {
            fftFillIndex = 0;
            // Copy to fftData and perform FFT
            std::copy(fftInputBuffer.begin(), fftInputBuffer.end(), fftData.begin());
            std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
            forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());
            fftDataReady.store(true, std::memory_order_release);
        }
    }

    // Apply sidechain compressor to the solfeggio-mixed output
    for (int ch = 0; ch < totalNumOutputChannels; ++ch) {
        sidechain.process(
            buffer.getWritePointer(ch),
            buffer.getReadPointer(ch),
            numSamples);
    }
}

void SolfeggioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SolfeggioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* SolfeggioProcessor::createEditor() {
    return new SolfeggioEditor(*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SolfeggioProcessor();
}
