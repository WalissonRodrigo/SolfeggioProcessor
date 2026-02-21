#include "PluginEditor.h"
#include "IconBinaryData.h"
#include "Constants.h"
#if JUCE_WINDOWS
#include "WindowsIconHelpers.h"
#endif

SolfeggioEditor::SolfeggioEditor(SolfeggioProcessor& p)
    : AudioProcessorEditor(p),
      processor(p),
      spectrumAnalyzer(p),
      autoModeBar(p),
      frequencyGrid(p)
{
    setLookAndFeel(&laf);
    setResizable(true, true);
    setResizeLimits(Solfeggio::Layout::minWidth, 750, 1400, 1100);
    setSize(Solfeggio::Layout::editorWidth, Solfeggio::Layout::editorHeight);

    // Taskbar / window icon
    if (auto* peer = getPeer()) {
        auto icon = juce::ImageFileFormat::loadFrom(
            IconData::icon_256_png, IconData::icon_256_png_size);
        if (icon.isValid()) {
            peer->setIcon(icon);
#if JUCE_WINDOWS
            WindowsIconHelper::setTaskbarIcon(this, icon);
#endif
        }
    }

    titleLabel.setText("SOLFEGGIO FREQUENCIES", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(22.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::gold);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    addAndMakeVisible(spectrumAnalyzer);
    addAndMakeVisible(autoModeBar);
    addAndMakeVisible(frequencyGrid);

    masterMixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterMixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(masterMixSlider);
    masterMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, Solfeggio::Params::masterMix.getParamID(), masterMixSlider);

    masterMixLabel.setText("MASTER MIX", juce::dontSendNotification);
    masterMixLabel.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
    masterMixLabel.setColour(juce::Label::textColourId, SolfeggioLookAndFeel::Colors::gold);
    addAndMakeVisible(masterMixLabel);

    startTimerHz(15);
}

SolfeggioEditor::~SolfeggioEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void SolfeggioEditor::timerCallback() {
    bool autoOn = processor.apvts.getRawParameterValue(
        Solfeggio::Params::autoMode.getParamID())->load() > 0.5f;
    autoModeBar.updateVisibility();
    frequencyGrid.updateVisuals(autoOn);
}

void SolfeggioEditor::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient(
        SolfeggioLookAndFeel::Colors::background,      0, 0,
        SolfeggioLookAndFeel::Colors::backgroundLight, bounds.getWidth(), bounds.getHeight(), true));
    g.fillRect(bounds);

    g.setColour(SolfeggioLookAndFeel::Colors::accent.withAlpha(0.5f));
    g.fillRect(0.0f, 0.0f, bounds.getWidth(), 2.0f);
}

void SolfeggioEditor::resized() {
    auto area = getLocalBounds().reduced(12);

    titleLabel.setBounds(area.removeFromTop(32));
    area.removeFromTop(4);

    const int specHeight = juce::jlimit(120, 250, area.getHeight() / 4);
    spectrumAnalyzer.setBounds(area.removeFromTop(specHeight).reduced(4, 2));
    area.removeFromTop(12);

    autoModeBar.setBounds(area.removeFromTop(35));
    area.removeFromTop(12);

    auto masterArea = area.removeFromBottom(45);
    masterMixLabel.setBounds(masterArea.removeFromLeft(110).reduced(0, 10));
    masterMixSlider.setBounds(masterArea.reduced(5, 8));

    area.removeFromBottom(8);
    frequencyGrid.setBounds(area);
}
