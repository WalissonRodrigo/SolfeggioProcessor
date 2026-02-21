#pragma once
#include <JuceHeader.h>
#include "SolfeggioProcessor.h"
#include "LookAndFeel.h"
#include "SpectrumAnalyzer.h"

//==============================================================================
class SolfeggioEditor : public juce::AudioProcessorEditor,
                        private juce::Timer {
public:
    explicit SolfeggioEditor(SolfeggioProcessor& p);
    ~SolfeggioEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

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

    // ===== Smart Auto Mode controls =====
    juce::ToggleButton autoModeButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoModeAttachment;

    juce::Slider cycleTimeSlider;
    juce::Label cycleTimeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cycleTimeAttachment;

    juce::Slider autoIntensitySlider;
    juce::Label autoIntensityLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> autoIntensityAttachment;

    juce::Label profileLabel;  // Shows current detected profile

    // Title label
    juce::Label titleLabel;

    void setupFrequencyControls();
    void setupMasterMix();
    void setupAutoControls();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SolfeggioEditor)
};
