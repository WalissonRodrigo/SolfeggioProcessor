#pragma once
#include <JuceHeader.h>
#include "SolfeggioProcessor.h"
#include "LookAndFeel.h"
#include "SpectrumAnalyzer.h"

//==============================================================================
class SolfeggioEditor : public juce::AudioProcessorEditor {
public:
    explicit SolfeggioEditor(SolfeggioProcessor& p);
    ~SolfeggioEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    SolfeggioProcessor& processor;
    SolfeggioLookAndFeel customLookAndFeel;

    // Spectrum visualizer
    SpectrumAnalyzer spectrumAnalyzer;

    // Frequency controls (10 knobs + 10 toggles)
    struct FrequencyControl {
        juce::Slider knob;
        juce::ToggleButton toggle;
        juce::Label nameLabel;
        juce::Label descLabel;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAttachment;
    };
    std::array<FrequencyControl, Solfeggio::NUM_FREQUENCIES> freqControls;

    // Master mix slider
    juce::Slider masterMixSlider;
    juce::Label masterMixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterMixAttachment;

    // Title label
    juce::Label titleLabel;

    void setupFrequencyControls();
    void setupMasterMix();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SolfeggioEditor)
};
