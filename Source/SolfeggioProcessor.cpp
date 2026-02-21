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
        return 0.0f;
    } else if (diff >= halfKnee) {
        return diff * (1.0f - 1.0f / r);
    } else {
        float x = diff + halfKnee;
        return (x * x) / (4.0f * knee) * (1.0f - 1.0f / r);
    }
}

float SidechainCompressor::computeEnvelopeRMS(const float* data, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        float filtered = sidechainFilter.processSample(data[i]);
        float sq = filtered * filtered;

        rmsSum -= rmsBuffer[static_cast<size_t>(rmsWritePos)];
        rmsBuffer[static_cast<size_t>(rmsWritePos)] = sq;
        rmsSum += sq;
        rmsWritePos = (rmsWritePos + 1) % static_cast<int>(rmsBuffer.size());
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

    attackCoeff  = 1.0f - std::exp(-1.0f / (static_cast<float>(currentSampleRate) * atk  * 0.001f));
    releaseCoeff = 1.0f - std::exp(-1.0f / (static_cast<float>(currentSampleRate) * rel * 0.001f));

    for (int i = 0; i < numSamples; ++i) {
        float filtered = sidechainFilter.processSample(sidechainInput[i]);
        float inputLevel = std::abs(filtered);

        float coeff = (inputLevel > envelopeLevel) ? attackCoeff : releaseCoeff;
        envelopeLevel += coeff * (inputLevel - envelopeLevel);

        float levelDb = juce::Decibels::gainToDecibels(envelopeLevel, -100.0f);

        float reductionDb = computeGainReduction(levelDb);
        float gainLin = juce::Decibels::decibelsToGain(-reductionDb);

        float compressed = data[i] * gainLin;
        data[i] = data[i] * (1.0f - wet) + compressed * wet;
    }
}

//==============================================================================
// SmartAutoEngine Implementation
//==============================================================================

SmartAutoEngine::SmartAutoEngine() {}

void SmartAutoEngine::prepare(double sr) {
    sampleRate = sr;

    // Bass band: lowpass at 300Hz
    auto bassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 300.0f);
    bassFilter.coefficients = bassCoeffs;
    bassFilter.reset();

    // Mid band: bandpass 300Hz–2kHz
    auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sr, 800.0f, 0.8f);
    midFilter.coefficients = midCoeffs;
    midFilter.reset();

    // High band: highpass at 2kHz
    auto highCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 2000.0f);
    highFilter.coefficients = highCoeffs;
    highFilter.reset();

    reset();
}

void SmartAutoEngine::reset() {
    smoothBass = 0.0f;
    smoothMid = 0.0f;
    smoothHigh = 0.0f;
    smoothTotal = 0.0f;
    cycleTimer = 0.0;
    crossfadeProgress = 1.0f;
    isCrossfading = false;
    currentProfile = MusicProfile::Quiet;
}

SmartAutoEngine::MusicProfile SmartAutoEngine::detectProfile() const {
    // Classify music based on band energy ratios
    float total = smoothBass + smoothMid + smoothHigh + 1e-10f;

    if (total < 0.001f)
        return MusicProfile::Quiet;

    float bassRatio = smoothBass / total;
    float midRatio = smoothMid / total;
    float highRatio = smoothHigh / total;

    if (bassRatio > 0.5f)
        return MusicProfile::BassHeavy;      // Funk, Hip-hop, EDM
    if (midRatio > 0.45f)
        return MusicProfile::MidFocused;      // MPB, Vocal, Sertanejo
    if (highRatio > 0.4f)
        return MusicProfile::Bright;          // Classical violin, bright pop
    return MusicProfile::FullSpectrum;        // Balanced music
}

void SmartAutoEngine::selectFrequenciesForProfile(
    MusicProfile profile, std::array<int, NUM_CYCLE_SLOTS>& selection)
{
    // Select 3 Solfeggio frequencies that HIDE BEST within the music
    // (frequencies in bands where music is loud are masked better)
    switch (profile) {
        case MusicProfile::BassHeavy:
            // Bass-heavy music: use higher solfeggio freqs (they hide above bass)
            // Indices: 5=528Hz, 6=639Hz, 7=741Hz
            selection = {5, 6, 7};
            break;
        case MusicProfile::MidFocused:
            // Mid-focused music: use mid-range solfeggio freqs (hide in vocals)
            // Indices: 3=417Hz, 4=432Hz, 5=528Hz
            selection = {3, 4, 5};
            break;
        case MusicProfile::Bright:
            // Bright music: use higher solfeggio freqs that blend with brightness
            // Indices: 7=741Hz, 8=852Hz, 9=963Hz
            selection = {7, 8, 9};
            break;
        case MusicProfile::FullSpectrum:
            // Balanced: spread across range
            // Indices: 2=396Hz, 5=528Hz, 8=852Hz
            selection = {2, 5, 8};
            break;
        case MusicProfile::Quiet:
            // When quiet: use low frequencies (most relaxing, less noticeable)
            // Indices: 0=174Hz, 1=285Hz, 2=396Hz
            selection = {0, 1, 2};
            break;
    }

    // Rotate to avoid always picking the same set:
    // Shift based on cycle slot to create variety
    int offset = currentCycleSlot % 3;
    if (offset > 0) {
        for (auto& idx : selection) {
            idx = (idx + offset) % Solfeggio::NUM_FREQUENCIES;
        }
    }
}

void SmartAutoEngine::analyzeBlock(const float* data, int numSamples) {
    // Compute band energies for this block
    float blockBass = 0.0f, blockMid = 0.0f, blockHigh = 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        float sample = data[i];

        float bass = bassFilter.processSample(sample);
        float mid  = midFilter.processSample(sample);
        float high = highFilter.processSample(sample);

        blockBass += bass * bass;
        blockMid  += mid * mid;
        blockHigh += high * high;
    }

    float invN = 1.0f / static_cast<float>(std::max(numSamples, 1));
    blockBass = std::sqrt(blockBass * invN);
    blockMid  = std::sqrt(blockMid * invN);
    blockHigh = std::sqrt(blockHigh * invN);

    // Smooth over time (exponential moving average)
    smoothBass = smoothCoeff * smoothBass + (1.0f - smoothCoeff) * blockBass;
    smoothMid  = smoothCoeff * smoothMid  + (1.0f - smoothCoeff) * blockMid;
    smoothHigh = smoothCoeff * smoothHigh + (1.0f - smoothCoeff) * blockHigh;
    smoothTotal = smoothBass + smoothMid + smoothHigh;

    // Update profile
    currentProfile = detectProfile();
}

void SmartAutoEngine::getTargetGains(
    std::array<float, Solfeggio::NUM_FREQUENCIES>& gains,
    float cycleTimeSec, float intensity)
{
    // Advance cycle timer
    double blockDuration = 1.0 / 60.0;  // Approximate: called ~60x/sec at 48kHz/1024 samples
    cycleTimer += blockDuration;

    // Handle crossfade progress
    if (isCrossfading) {
        crossfadeProgress += static_cast<float>(blockDuration / crossfadeDurationSec);
        if (crossfadeProgress >= 1.0f) {
            crossfadeProgress = 1.0f;
            isCrossfading = false;
            activeFreqs = nextFreqs;
        }
    }

    // Time to switch frequencies?
    if (cycleTimer >= static_cast<double>(cycleTimeSec) && !isCrossfading) {
        cycleTimer = 0.0;
        currentCycleSlot++;

        // Select new frequencies based on current profile
        selectFrequenciesForProfile(currentProfile, nextFreqs);

        // Only start crossfade if the selection actually changed
        if (nextFreqs != activeFreqs) {
            isCrossfading = true;
            crossfadeProgress = 0.0f;
        }
    }

    // Build smooth gains
    gains.fill(0.0f);

    // Active frequencies (fade OUT during crossfade)
    float activeGain = isCrossfading ? (1.0f - crossfadeProgress) : 1.0f;
    for (int idx : activeFreqs) {
        gains[static_cast<size_t>(idx)] += activeGain;
    }

    // Next frequencies (fade IN during crossfade)
    if (isCrossfading) {
        float nextGain = crossfadeProgress;
        for (int idx : nextFreqs) {
            gains[static_cast<size_t>(idx)] += nextGain;
        }
    }

    // Apply intensity and adaptive volume
    // When music is loud, reduce solfeggio volume for subliminal effect
    float adaptiveVolume = 1.0f;
    if (smoothTotal > 0.01f) {
        // The louder the music, the more the solfeggio hides
        adaptiveVolume = juce::jlimit(0.2f, 1.0f,
            0.5f / (smoothTotal + 0.5f));
    }

    float finalScale = intensity * adaptiveVolume;
    for (auto& g : gains) {
        g = juce::jlimit(0.0f, 1.0f, g * finalScale);
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

    // ===== Smart Auto Mode parameters =====
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"AutoMode", 1}, "Auto Mode", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"CycleTime", 1}, "Cycle Time",
        juce::NormalisableRange<float>(15.0f, 120.0f, 1.0f), 45.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(static_cast<int>(v)) + "s"; },
        [](const juce::String& s) { return s.getFloatValue(); }));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"AutoIntensity", 1}, "Auto Intensity",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + "%"; },
        [](const juce::String& s) { return s.getFloatValue(); }));

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
    for (auto& ag : autoSmoothedGains) {
        ag.reset(sampleRate, 0.05); // 50ms smoothing for auto mode (smoother)
    }
    smoothedMix.reset(sampleRate, 0.02);

    // Setup auto engine
    autoEngine.prepare(sampleRate);

    // Reset FFT
    fftFillIndex = 0;
    fftInputBuffer.fill(0.0f);
    fftData.fill(0.0f);
}

void SolfeggioProcessor::releaseResources() {
    sidechain.reset();
    autoEngine.reset();
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

    // Read auto mode parameters
    bool autoMode = apvts.getRawParameterValue("AutoMode")->load() > 0.5f;
    float cycleTime = apvts.getRawParameterValue("CycleTime")->load();
    float autoIntensity = apvts.getRawParameterValue("AutoIntensity")->load() / 100.0f;

    // Determine frequency gains
    std::array<float, Solfeggio::NUM_FREQUENCIES> targetGains{};

    if (autoMode) {
        // ===== SMART AUTO MODE =====
        // Analyze the input audio (channel 0)
        autoEngine.analyzeBlock(buffer.getReadPointer(0), numSamples);

        // Get smart target gains
        autoEngine.getTargetGains(targetGains, cycleTime, autoIntensity);

        // Set smoothed gains for auto mode
        for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
            autoSmoothedGains[static_cast<size_t>(i)].setTargetValue(targetGains[static_cast<size_t>(i)]);
        }
    } else {
        // ===== MANUAL MODE =====
        for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
            auto freqStr = juce::String(Solfeggio::Frequencies[static_cast<size_t>(i)], 0);
            float g = apvts.getRawParameterValue(freqStr + "Hz_Gain")->load();
            bool  e = apvts.getRawParameterValue(freqStr + "Hz_On")->load() > 0.5f;
            float gain = e ? g : 0.0f;
            smoothedGains[static_cast<size_t>(i)].setTargetValue(gain);
        }
    }

    // Generate solfeggio signal and mix
    for (int sample = 0; sample < numSamples; ++sample) {
        float currentMix = smoothedMix.getNextValue();

        // Accumulate all active oscillators
        float solfeggioSample = 0.0f;
        for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
            float gain;
            if (autoMode) {
                gain = autoSmoothedGains[static_cast<size_t>(i)].getNextValue();
            } else {
                gain = smoothedGains[static_cast<size_t>(i)].getNextValue();
            }

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
