#pragma once
#include <JuceHeader.h>
#include "SolfeggioProcessor.h"
#include "LookAndFeel.h"
#include "SpectrumAnalyzer.h"
#include "AutoModeBar.h"
#include "FrequencyGrid.h"

// ============================================================================
// SolfeggioEditor  (GUI / View layer)
// Responsibility: Owns and arranges top-level visual components.
// Zero audio processing logic lives here.
// ============================================================================
class SolfeggioEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit SolfeggioEditor(SolfeggioProcessor& p);
    ~SolfeggioEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    SolfeggioProcessor& processor;
    SolfeggioLookAndFeel laf;

    // Top-level visual components (ordered top → bottom in the layout)
    juce::Label       titleLabel;
    SpectrumAnalyzer  spectrumAnalyzer;
    AutoModeBar       autoModeBar;
    FrequencyGrid     frequencyGrid;

    juce::Slider masterMixSlider;
    juce::Label  masterMixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterMixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SolfeggioEditor)
};
