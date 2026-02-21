#include "SolfeggioProcessor.h"
#include "PluginEditor.h"

SolfeggioProcessor::SolfeggioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{}

void SolfeggioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    engine.prepare(sampleRate, samplesPerBlock);
}

void SolfeggioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;

    engine.setSidechainParams(
        apvts.getRawParameterValue(Solfeggio::Params::scAttack.getParamID())->load(),
        apvts.getRawParameterValue(Solfeggio::Params::scRelease.getParamID())->load(),
        apvts.getRawParameterValue(Solfeggio::Params::scDryWet.getParamID())->load()
    );

    std::array<float, Solfeggio::NUM_FREQUENCIES> manualGains;
    for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
        float freq = Solfeggio::Frequencies[static_cast<size_t>(i)];
        float gain = apvts.getRawParameterValue(Solfeggio::Params::getGainID(freq))->load();
        bool  on   = apvts.getRawParameterValue(Solfeggio::Params::getOnID(freq))->load() > 0.5f;
        manualGains[static_cast<size_t>(i)] = on ? gain : 0.0f;
    }

    engine.process(
        buffer,
        apvts.getRawParameterValue(Solfeggio::Params::autoMode.getParamID())->load() > 0.5f,
        apvts.getRawParameterValue(Solfeggio::Params::cycleTime.getParamID())->load(),
        apvts.getRawParameterValue(Solfeggio::Params::autoIntensity.getParamID())->load() / 100.0f,
        manualGains,
        apvts.getRawParameterValue(Solfeggio::Params::masterMix.getParamID())->load() / 100.0f
    );
}

bool SolfeggioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

juce::AudioProcessorValueTreeState::ParameterLayout SolfeggioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
        float freq = Solfeggio::Frequencies[static_cast<size_t>(i)];
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ Solfeggio::Params::getGainID(freq), 1 },
            juce::String(freq, 0) + " Hz Gain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ Solfeggio::Params::getOnID(freq), 1 },
            juce::String(freq, 0) + " Hz On", false));
    }

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        Solfeggio::Params::masterMix,      "Master Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 15.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        Solfeggio::Params::scAttack,       "SC Attack",
        juce::NormalisableRange<float>(1.0f, 1000.0f, 1.0f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        Solfeggio::Params::scRelease,      "SC Release",
        juce::NormalisableRange<float>(1.0f, 1000.0f, 1.0f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        Solfeggio::Params::scDryWet,       "SC Dry/Wet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        Solfeggio::Params::autoMode,       "Auto Mode", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        Solfeggio::Params::cycleTime,      "Cycle Time",
        juce::NormalisableRange<float>(15.0f, 120.0f, 1.0f), 45.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        Solfeggio::Params::autoIntensity,  "Auto Intensity",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f));

    return { params.begin(), params.end() };
}

void SolfeggioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void SolfeggioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* SolfeggioProcessor::createEditor() {
    return new SolfeggioEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SolfeggioProcessor();
}
