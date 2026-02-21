#pragma once
#include <JuceHeader.h>
#include "SolfeggioProcessor.h"
#include "LookAndFeel.h"

//==============================================================================
// Real-time FFT spectrum visualizer with Solfeggio frequency markers
//==============================================================================
class SpectrumAnalyzer : public juce::Component, private juce::Timer {
public:
    explicit SpectrumAnalyzer(SolfeggioProcessor& proc)
        : processor(proc)
    {
        startTimerHz(30); // 30fps
        scopeData.fill(0.0f);
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        auto w = bounds.getWidth();
        auto h = bounds.getHeight();

        // Background with gradient
        auto bgGradient = juce::ColourGradient(
            SolfeggioLookAndFeel::Colors::background, 0, 0,
            SolfeggioLookAndFeel::Colors::backgroundLight, 0, h, false);
        g.setGradientFill(bgGradient);
        g.fillRoundedRectangle(bounds, 8.0f);

        // Border
        g.setColour(SolfeggioLookAndFeel::Colors::surface);
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

        // Draw frequency grid lines
        g.setColour(SolfeggioLookAndFeel::Colors::surface.withAlpha(0.3f));
        for (float freq : {100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f}) {
            float x = freqToX(freq, w);
            g.drawVerticalLine(static_cast<int>(x + bounds.getX()), bounds.getY(), bounds.getBottom());
        }

        // Draw Solfeggio frequency markers
        for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
            float freq = Solfeggio::Frequencies[static_cast<size_t>(i)];
            float x = freqToX(freq, w) + bounds.getX();

            // Marker line
            g.setColour(SolfeggioLookAndFeel::Colors::gold.withAlpha(0.4f));
            g.drawVerticalLine(static_cast<int>(x), bounds.getY() + 15, bounds.getBottom() - 5);

            // Frequency label
            g.setColour(SolfeggioLookAndFeel::Colors::gold.withAlpha(0.7f));
            g.setFont(juce::Font(9.0f));
            g.drawText(juce::String(static_cast<int>(freq)),
                       static_cast<int>(x - 15), static_cast<int>(bounds.getY() + 2),
                       30, 12, juce::Justification::centred);
        }

        // Draw spectrum path
        juce::Path spectrumPath;
        bool pathStarted = false;
        float padding = 10.0f;
        float drawH = h - padding * 2;

        for (int i = 0; i < scopeSize; ++i) {
            float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(scopeSize)) * w;
            float y = bounds.getBottom() - padding
                     - scopeData[static_cast<size_t>(i)] * drawH;
            y = juce::jlimit(bounds.getY() + padding, bounds.getBottom() - padding, y);

            if (!pathStarted) {
                spectrumPath.startNewSubPath(x, y);
                pathStarted = true;
            } else {
                spectrumPath.lineTo(x, y);
            }
        }

        // Filled area under curve
        if (pathStarted) {
            juce::Path fillPath(spectrumPath);
            fillPath.lineTo(bounds.getRight(), bounds.getBottom() - padding);
            fillPath.lineTo(bounds.getX(), bounds.getBottom() - padding);
            fillPath.closeSubPath();

            auto fillGradient = juce::ColourGradient(
                SolfeggioLookAndFeel::Colors::accent.withAlpha(0.3f), 0, bounds.getY(),
                SolfeggioLookAndFeel::Colors::accent.withAlpha(0.02f), 0, bounds.getBottom(), false);
            g.setGradientFill(fillGradient);
            g.fillPath(fillPath);

            // Spectrum line stroke
            auto lineGradient = juce::ColourGradient(
                SolfeggioLookAndFeel::Colors::accent, 0, bounds.getY(),
                SolfeggioLookAndFeel::Colors::gold, 0, bounds.getBottom(), false);
            g.setGradientFill(lineGradient);
            g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
        }
    }

    void resized() override {}

private:
    void timerCallback() override {
        if (processor.fftDataReady.exchange(false, std::memory_order_acquire)) {
            // Convert FFT data to scope display
            auto mindB = -80.0f;
            auto maxdB = 0.0f;

            for (int i = 0; i < scopeSize; ++i) {
                // Map scope index to FFT bin (logarithmic)
                auto freq = mapScopeIndexToFreq(i);
                auto fftBin = static_cast<int>(freq / (44100.0f / static_cast<float>(SolfeggioProcessor::fftSize)));
                fftBin = juce::jlimit(0, SolfeggioProcessor::fftSize / 2 - 1, fftBin);

                auto level = juce::Decibels::gainToDecibels(
                    processor.fftData[static_cast<size_t>(fftBin)], mindB);

                auto normalised = juce::jmap(level, mindB, maxdB, 0.0f, 1.0f);

                // Smooth the display
                scopeData[static_cast<size_t>(i)] =
                    scopeData[static_cast<size_t>(i)] * 0.7f + normalised * 0.3f;
            }
        }
        repaint();
    }

    float freqToX(float freq, float width) const {
        // Logarithmic mapping 20Hz..20kHz → 0..width
        auto minLog = std::log10(20.0f);
        auto maxLog = std::log10(20000.0f);
        return (std::log10(freq) - minLog) / (maxLog - minLog) * width;
    }

    float mapScopeIndexToFreq(int index) const {
        auto minLog = std::log10(20.0f);
        auto maxLog = std::log10(20000.0f);
        auto t = static_cast<float>(index) / static_cast<float>(scopeSize);
        return std::pow(10.0f, minLog + t * (maxLog - minLog));
    }

    SolfeggioProcessor& processor;

    static constexpr int scopeSize = 512;
    std::array<float, scopeSize> scopeData{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};
