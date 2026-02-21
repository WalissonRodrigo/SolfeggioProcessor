#include "PluginEditor.h"

//==============================================================================
SolfeggioEditor::SolfeggioEditor(SolfeggioProcessor& p)
    : AudioProcessorEditor(p),
      processor(p),
      spectrumAnalyzer(p)
{
    setLookAndFeel(&customLookAndFeel);
    setResizable(true, true);
    setResizeLimits(700, 560, 1400, 1100);
    setSize(750, 600);

    // Title
    titleLabel.setText("SOLFEGGIO FREQUENCIES", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::gold);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // Spectrum analyzer
    addAndMakeVisible(spectrumAnalyzer);

    // Setup sub-sections
    setupAutoControls();
    setupFrequencyControls();
    setupMasterMix();

    // Start timer for auto-mode GUI animation (10fps)
    startTimerHz(10);
}

SolfeggioEditor::~SolfeggioEditor() {
    setLookAndFeel(nullptr);
    stopTimer();
}

void SolfeggioEditor::setupAutoControls() {
    // AUTO toggle button
    autoModeButton.setButtonText("AUTO");
    autoModeButton.setColour(juce::ToggleButton::textColourId, SolfeggioLookAndFeel::Colors::gold);
    autoModeButton.setColour(juce::ToggleButton::tickColourId, SolfeggioLookAndFeel::Colors::accent);
    addAndMakeVisible(autoModeButton);
    autoModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "AutoMode", autoModeButton);

    // Cycle time slider
    cycleTimeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    cycleTimeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(cycleTimeSlider);
    cycleTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "CycleTime", cycleTimeSlider);

    cycleTimeLabel.setText("CYCLE", juce::dontSendNotification);
    cycleTimeLabel.setFont(juce::Font(10.0f));
    cycleTimeLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::textSecondary);
    cycleTimeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(cycleTimeLabel);

    // Intensity slider
    autoIntensitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    autoIntensitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(autoIntensitySlider);
    autoIntensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "AutoIntensity", autoIntensitySlider);

    autoIntensityLabel.setText("INTENSITY", juce::dontSendNotification);
    autoIntensityLabel.setFont(juce::Font(10.0f));
    autoIntensityLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::textSecondary);
    autoIntensityLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(autoIntensityLabel);

    // Profile display label
    profileLabel.setText("Profile: --", juce::dontSendNotification);
    profileLabel.setFont(juce::Font(11.0f, juce::Font::italic));
    profileLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::accent);
    profileLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(profileLabel);
}

void SolfeggioEditor::setupFrequencyControls() {
    for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
        auto& ctrl = freqControls[static_cast<size_t>(i)];
        auto freqStr = juce::String(Solfeggio::Frequencies[static_cast<size_t>(i)], 0);

        // Rotary knob
        ctrl.knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        ctrl.knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(ctrl.knob);
        ctrl.gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, freqStr + "Hz_Gain", ctrl.knob);

        // Toggle button
        addAndMakeVisible(ctrl.toggle);
        ctrl.toggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.apvts, freqStr + "Hz_On", ctrl.toggle);

        // Frequency name label
        ctrl.nameLabel.setText(Solfeggio::FrequencyNames[i], juce::dontSendNotification);
        ctrl.nameLabel.setFont(juce::Font(12.0f, juce::Font::bold));
        ctrl.nameLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::textPrimary);
        ctrl.nameLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(ctrl.nameLabel);

        // Description label
        ctrl.descLabel.setText(Solfeggio::FrequencyDescriptions[i], juce::dontSendNotification);
        ctrl.descLabel.setFont(juce::Font(9.0f));
        ctrl.descLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::textSecondary);
        ctrl.descLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(ctrl.descLabel);
    }
}

void SolfeggioEditor::setupMasterMix() {
    masterMixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterMixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(masterMixSlider);
    masterMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "MasterMix", masterMixSlider);

    masterMixLabel.setText("MASTER MIX", juce::dontSendNotification);
    masterMixLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    masterMixLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::gold);
    masterMixLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(masterMixLabel);
}

void SolfeggioEditor::timerCallback() {
    // Update profile display
    auto profile = processor.autoEngine.getCurrentProfile();
    juce::String profileText;
    switch (profile) {
        case SmartAutoEngine::MusicProfile::BassHeavy:    profileText = "Bass Heavy (Funk/EDM)"; break;
        case SmartAutoEngine::MusicProfile::MidFocused:   profileText = "Mid Focused (Vocal/MPB)"; break;
        case SmartAutoEngine::MusicProfile::Bright:       profileText = "Bright (Classical/Pop)"; break;
        case SmartAutoEngine::MusicProfile::FullSpectrum:  profileText = "Full Spectrum"; break;
        case SmartAutoEngine::MusicProfile::Quiet:        profileText = "Quiet / No Signal"; break;
    }
    profileLabel.setText("Profile: " + profileText, juce::dontSendNotification);

    // Toggle opacity of manual controls when auto is on
    bool autoOn = processor.apvts.getRawParameterValue("AutoMode")->load() > 0.5f;
    float knobAlpha = autoOn ? 0.35f : 1.0f;
    for (auto& ctrl : freqControls) {
        ctrl.knob.setAlpha(knobAlpha);
        ctrl.toggle.setAlpha(knobAlpha);
        ctrl.knob.setEnabled(!autoOn);
        ctrl.toggle.setEnabled(!autoOn);
    }

    // Show/hide auto controls
    cycleTimeSlider.setVisible(autoOn);
    cycleTimeLabel.setVisible(autoOn);
    autoIntensitySlider.setVisible(autoOn);
    autoIntensityLabel.setVisible(autoOn);
    profileLabel.setVisible(autoOn);
}

void SolfeggioEditor::paint(juce::Graphics& g) {
    // Background gradient
    auto bounds = getLocalBounds().toFloat();
    auto gradient = juce::ColourGradient(
        SolfeggioLookAndFeel::Colors::background, 0, 0,
        SolfeggioLookAndFeel::Colors::backgroundLight, bounds.getWidth(), bounds.getHeight(),
        true);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    // Subtle accent line at top
    g.setColour(SolfeggioLookAndFeel::Colors::accent.withAlpha(0.5f));
    g.fillRect(0.0f, 0.0f, bounds.getWidth(), 2.0f);
}

void SolfeggioEditor::resized() {
    auto area = getLocalBounds().reduced(10);
    int w = area.getWidth();

    // Title (30px)
    titleLabel.setBounds(area.removeFromTop(30));

    // Spectrum analyzer (height ~25% of remaining)
    int specHeight = juce::jmax(80, area.getHeight() / 5);
    spectrumAnalyzer.setBounds(area.removeFromTop(specHeight).reduced(5, 2));

    area.removeFromTop(4);

    // ===== Auto mode bar (35px) =====
    auto autoBar = area.removeFromTop(35);
    int autoToggleW = 80;
    autoModeButton.setBounds(autoBar.removeFromLeft(autoToggleW));

    bool autoOn = processor.apvts.getRawParameterValue("AutoMode")->load() > 0.5f;
    if (autoOn) {
        auto remaining = autoBar;
        profileLabel.setBounds(remaining.removeFromLeft(180));
        cycleTimeLabel.setBounds(remaining.removeFromLeft(45));
        cycleTimeSlider.setBounds(remaining.removeFromLeft(juce::jmin(remaining.getWidth() / 2, 160)));
        remaining.removeFromLeft(5);
        autoIntensityLabel.setBounds(remaining.removeFromLeft(65));
        autoIntensitySlider.setBounds(remaining.removeFromLeft(juce::jmin(remaining.getWidth(), 160)));
    }

    area.removeFromTop(4);

    // ===== Frequency controls — 2 rows of 5 =====
    int knobRows = 2;
    int knobCols = 5;
    int knobAreaHeight = area.getHeight() - 45; // Reserve 45px for master mix
    int rowHeight = knobAreaHeight / knobRows;
    int colWidth = w / knobCols;

    for (int row = 0; row < knobRows; ++row) {
        for (int col = 0; col < knobCols; ++col) {
            int idx = row * knobCols + col;
            auto& ctrl = freqControls[static_cast<size_t>(idx)];

            int x = area.getX() + col * colWidth;
            int y = area.getY() + row * rowHeight;

            int knobSize = juce::jmin(colWidth - 20, rowHeight - 50);
            knobSize = juce::jmax(40, knobSize);

            int knobX = x + (colWidth - knobSize) / 2;
            int knobY = y + 2;

            ctrl.knob.setBounds(knobX, knobY, knobSize, knobSize);
            ctrl.toggle.setBounds(knobX + knobSize - 4, knobY - 2, 20, 20);
            ctrl.nameLabel.setBounds(x, knobY + knobSize + 2, colWidth, 16);
            ctrl.descLabel.setBounds(x, knobY + knobSize + 16, colWidth, 14);
        }
    }

    // ===== Master mix at bottom =====
    auto masterArea = area.withTrimmedTop(knobAreaHeight);
    int labelW = 100;
    masterMixLabel.setBounds(masterArea.removeFromLeft(labelW).reduced(0, 8));
    masterMixSlider.setBounds(masterArea.reduced(5, 8));
}
