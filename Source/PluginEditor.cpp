#include "PluginEditor.h"

//==============================================================================
SolfeggioEditor::SolfeggioEditor(SolfeggioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      spectrumAnalyzer(p)
{
    setLookAndFeel(&customLookAndFeel);
    setSize(900, 620);
    setResizable(true, true);
    setResizeLimits(700, 500, 1400, 900);

    // Title
    titleLabel.setText("SOLFEGGIO FREQUENCIES", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::gold);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // Spectrum analyzer
    addAndMakeVisible(spectrumAnalyzer);

    // Frequency controls
    setupFrequencyControls();

    // Master mix
    setupMasterMix();
}

SolfeggioEditor::~SolfeggioEditor() {
    setLookAndFeel(nullptr);
}

void SolfeggioEditor::setupFrequencyControls() {
    for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
        auto& ctrl = freqControls[static_cast<size_t>(i)];
        auto freqStr = juce::String(Solfeggio::Frequencies[static_cast<size_t>(i)], 0);

        // Rotary knob
        ctrl.knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        ctrl.knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        ctrl.knob.setPopupDisplayEnabled(true, true, this);
        addAndMakeVisible(ctrl.knob);

        ctrl.gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, freqStr + "Hz_Gain", ctrl.knob);

        // Toggle button
        ctrl.toggle.setClickingTogglesState(true);
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
    masterMixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 24);
    masterMixSlider.setColour(juce::Slider::textBoxTextColourId,
                              SolfeggioLookAndFeel::Colors::textPrimary);
    masterMixSlider.setColour(juce::Slider::textBoxOutlineColourId,
                              juce::Colours::transparentBlack);
    addAndMakeVisible(masterMixSlider);

    masterMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "MasterMix", masterMixSlider);

    masterMixLabel.setText("MASTER MIX", juce::dontSendNotification);
    masterMixLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    masterMixLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::gold);
    masterMixLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(masterMixLabel);
}

//==============================================================================
void SolfeggioEditor::paint(juce::Graphics& g) {
    // Full background gradient
    auto bounds = getLocalBounds().toFloat();
    auto bgGradient = juce::ColourGradient(
        SolfeggioLookAndFeel::Colors::background, 0, 0,
        juce::Colour(0xff0d1117), 0, bounds.getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Subtle top accent line
    auto accentGrad = juce::ColourGradient(
        juce::Colours::transparentBlack, 0, 0,
        juce::Colours::transparentBlack, bounds.getWidth(), 0, false);
    accentGrad.addColour(0.2, SolfeggioLookAndFeel::Colors::accent.withAlpha(0.4f));
    accentGrad.addColour(0.5, SolfeggioLookAndFeel::Colors::gold.withAlpha(0.6f));
    accentGrad.addColour(0.8, SolfeggioLookAndFeel::Colors::accent.withAlpha(0.4f));
    g.setGradientFill(accentGrad);
    g.fillRect(0.0f, 0.0f, bounds.getWidth(), 3.0f);

    // Section separator line above knobs
    auto knobAreaTop = bounds.getHeight() * 0.48f;
    g.setColour(SolfeggioLookAndFeel::Colors::surface.withAlpha(0.5f));
    g.drawHorizontalLine(static_cast<int>(knobAreaTop), 20.0f, bounds.getWidth() - 20.0f);

    // Section separator above master mix
    auto masterTop = bounds.getHeight() - 70.0f;
    g.drawHorizontalLine(static_cast<int>(masterTop), 20.0f, bounds.getWidth() - 20.0f);
}

void SolfeggioEditor::resized() {
    auto bounds = getLocalBounds();
    auto margin = 15;

    // Title area — top 40px
    titleLabel.setBounds(bounds.getX(), margin, bounds.getWidth(), 30);

    // Spectrum analyzer — from title bottom to ~45% height
    auto spectrumTop = margin + 35;
    auto spectrumHeight = static_cast<int>(bounds.getHeight() * 0.35f);
    spectrumAnalyzer.setBounds(margin, spectrumTop,
                                bounds.getWidth() - margin * 2, spectrumHeight);

    // Frequency controls area — middle section
    auto knobAreaTop = spectrumTop + spectrumHeight + 15;
    auto knobAreaHeight = bounds.getHeight() - knobAreaTop - 75; // leave room for master
    auto knobWidth = (bounds.getWidth() - margin * 2) / 5;   // 5 columns
    auto knobRows = 2;
    auto knobRowHeight = knobAreaHeight / knobRows;

    for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
        auto& ctrl = freqControls[static_cast<size_t>(i)];
        int col = i % 5;
        int row = i / 5;

        auto x = margin + col * knobWidth;
        auto y = knobAreaTop + row * knobRowHeight;
        auto cellW = knobWidth;
        auto knobSize = juce::jmin(static_cast<int>(cellW * 0.65f), knobRowHeight - 45);

        // Center knob in cell
        auto knobX = x + (cellW - knobSize) / 2;
        ctrl.knob.setBounds(knobX, y, knobSize, knobSize);

        // Toggle button (right side of knob)
        auto toggleSize = 20;
        ctrl.toggle.setBounds(knobX + knobSize - 5, y, toggleSize, toggleSize);

        // Name label below knob
        ctrl.nameLabel.setBounds(x, y + knobSize + 2, cellW, 15);

        // Description below name
        ctrl.descLabel.setBounds(x, y + knobSize + 16, cellW, 12);
    }

    // Master mix — bottom strip
    auto masterY = bounds.getHeight() - 55;
    masterMixLabel.setBounds(margin, masterY, 120, 30);
    masterMixSlider.setBounds(margin + 120, masterY, bounds.getWidth() - margin * 2 - 120, 30);
}
