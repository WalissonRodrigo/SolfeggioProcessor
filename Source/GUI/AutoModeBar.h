#pragma once
#include "Constants.h"
#include "LookAndFeel.h"
#include "SmartAutoEngine.h"
#include "SolfeggioProcessor.h"
#include <JuceHeader.h>

class AutoModeBar : public juce::Component {
public:
  AutoModeBar(SolfeggioProcessor &p) : processor(p) {
    autoModeButton.setButtonText("AUTO");
    autoModeButton.setColour(juce::ToggleButton::textColourId,
                             SolfeggioLookAndFeel::Colors::gold);
    autoModeButton.setColour(juce::ToggleButton::tickColourId,
                             SolfeggioLookAndFeel::Colors::accent);
    addAndMakeVisible(autoModeButton);
    autoModeAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.apvts, Solfeggio::Params::autoMode.getParamID(),
            autoModeButton);

    cycleTimeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    cycleTimeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(cycleTimeSlider);
    cycleTimeAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, Solfeggio::Params::cycleTime.getParamID(),
            cycleTimeSlider);

    cycleTimeLabel.setText("CYCLE", juce::dontSendNotification);
    cycleTimeLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    cycleTimeLabel.setColour(juce::Label::textColourId,
                             SolfeggioLookAndFeel::Colors::textSecondary);
    cycleTimeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(cycleTimeLabel);

    autoIntensitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    autoIntensitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50,
                                        20);
    addAndMakeVisible(autoIntensitySlider);
    autoIntensityAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, Solfeggio::Params::autoIntensity.getParamID(),
            autoIntensitySlider);

    autoIntensityLabel.setText("INTENSITY", juce::dontSendNotification);
    autoIntensityLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    autoIntensityLabel.setColour(juce::Label::textColourId,
                                 SolfeggioLookAndFeel::Colors::textSecondary);
    autoIntensityLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(autoIntensityLabel);

    profileLabel.setText("Profile: --", juce::dontSendNotification);
    profileLabel.setFont(
        juce::Font(juce::FontOptions(11.0f).withStyle("Italic")));
    profileLabel.setColour(juce::Label::textColourId,
                           SolfeggioLookAndFeel::Colors::accent);
    profileLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(profileLabel);
  }

  void updateVisibility() {
    bool autoOn =
        processor.apvts
            .getRawParameterValue(Solfeggio::Params::autoMode.getParamID())
            ->load() > 0.5f;
    cycleTimeSlider.setVisible(autoOn);
    cycleTimeLabel.setVisible(autoOn);
    autoIntensitySlider.setVisible(autoOn);
    autoIntensityLabel.setVisible(autoOn);
    profileLabel.setVisible(autoOn);

    if (autoOn) {
      auto profile = processor.engine.getAutoEngine().getCurrentProfile();
      profileLabel.setText("Profile: " + getProfileText(profile),
                           juce::dontSendNotification);
    }
  }

  void resized() override {
    auto area = getLocalBounds();
    autoModeButton.setBounds(area.removeFromLeft(80));

    if (autoModeButton.getToggleState()) {
      profileLabel.setBounds(area.removeFromLeft(180));
      cycleTimeLabel.setBounds(area.removeFromLeft(45));
      cycleTimeSlider.setBounds(
          area.removeFromLeft(juce::jmin(area.getWidth() / 2, 160)));
      area.removeFromLeft(5);
      autoIntensityLabel.setBounds(area.removeFromLeft(65));
      autoIntensitySlider.setBounds(
          area.removeFromLeft(juce::jmin(area.getWidth(), 160)));
    }
  }

private:
  juce::String getProfileText(SmartAutoEngine::MusicProfile p) {
    switch (p) {
    case SmartAutoEngine::MusicProfile::BassHeavy:
      return "Bass Heavy (Funk/EDM)";
    case SmartAutoEngine::MusicProfile::MidFocused:
      return "Mid Focused (Vocal/MPB)";
    case SmartAutoEngine::MusicProfile::Bright:
      return "Bright (Classical/Pop)";
    case SmartAutoEngine::MusicProfile::FullSpectrum:
      return "Full Spectrum";
    case SmartAutoEngine::MusicProfile::Quiet:
      return "Quiet / No Signal";
    }
    return "Unknown";
  }

  SolfeggioProcessor &processor;
  juce::ToggleButton autoModeButton;
  juce::Slider cycleTimeSlider, autoIntensitySlider;
  juce::Label cycleTimeLabel, autoIntensityLabel, profileLabel;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      autoModeAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      cycleTimeAttachment, autoIntensityAttachment;
};
