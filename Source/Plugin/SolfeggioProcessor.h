#pragma once
#include <JuceHeader.h>
#include "Constants.h"
#include "SolfeggioEngine.h"

// ============================================================================
// SolfeggioProcessor  (Plugin / Controller layer)
// Responsibility: JUCE plugin lifecycle, parameter layout, APVTS bridge.
// All DSP is delegated to SolfeggioEngine (Dependency Inversion Principle).
// ============================================================================
class SolfeggioProcessor : public juce::AudioProcessor {
public:
    SolfeggioProcessor();
    ~SolfeggioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    // Explicitly pull in double-buffer overload to satisfy JUCE 8's hidden-virtual check
    using juce::AudioProcessor::processBlock;
    void releaseResources() override { engine.reset(); }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    SolfeggioEngine engine;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SolfeggioProcessor)
};
