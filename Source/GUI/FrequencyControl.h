#pragma once
#include <JuceHeader.h>
#include "Constants.h"
#include "LookAndFeel.h"
#include "SolfeggioProcessor.h"

// ============================================================================
// FrequencyControl (GUI/View layer)
// Responsibility: Owns and manages the UI for a single Solfeggio oscillator
// — gain knob, on/off toggle, name label, and description label.
// ============================================================================
class FrequencyControl : public juce::Component {
public:
    FrequencyControl(SolfeggioProcessor& p, int index)
        : processor(p), freqIndex(index)
    {
        const auto idx     = static_cast<size_t>(index);
        const float freq   = Solfeggio::Frequencies[idx];

        knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(knob);
        gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, Solfeggio::Params::getGainID(freq), knob);

        addAndMakeVisible(toggle);
        toggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.apvts, Solfeggio::Params::getOnID(freq), toggle);

        nameLabel.setText(Solfeggio::FrequencyNames[idx], juce::dontSendNotification);
        nameLabel.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
        nameLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::textPrimary);
        nameLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(nameLabel);

        descLabel.setText(Solfeggio::FrequencyDescriptions[idx], juce::dontSendNotification);
        descLabel.setFont(juce::Font(juce::FontOptions(9.0f)));
        descLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::textSecondary);
        descLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(descLabel);
    }

    void resized() override {
        auto area     = getLocalBounds();
        int knobSize  = juce::jmin(area.getWidth() - 20, area.getHeight() - 40);
        knob.setBounds((area.getWidth() - knobSize) / 2, 2, knobSize, knobSize);
        toggle.setBounds(knob.getRight() - 4, knob.getY() - 2, 20, 20);
        nameLabel.setBounds(0, knob.getBottom() + 2,  area.getWidth(), 16);
        descLabel.setBounds(0, knob.getBottom() + 18, area.getWidth(), 14);
    }

    void setAlphaAndEnabled(float alpha, bool enabled) {
        knob.setAlpha(alpha);
        toggle.setAlpha(alpha);
        knob.setEnabled(enabled);
        toggle.setEnabled(enabled);
    }

private:
    SolfeggioProcessor& processor;
    int freqIndex;

    juce::Slider       knob;
    juce::ToggleButton toggle;
    juce::Label        nameLabel, descLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyControl)
};
