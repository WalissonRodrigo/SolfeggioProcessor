#pragma once
#include <JuceHeader.h>

//==============================================================================
// Custom dark theme with purple/gold accents for the Solfeggio plugin
//==============================================================================
class SolfeggioLookAndFeel : public juce::LookAndFeel_V4 {
public:
    // Color palette
    struct Colors {
        static inline const juce::Colour background      {0xff1a1a2e};
        static inline const juce::Colour backgroundLight {0xff16213e};
        static inline const juce::Colour surface         {0xff0f3460};
        static inline const juce::Colour surfaceLight    {0xff1a4a7a};
        static inline const juce::Colour accent          {0xff7b2ff7};
        static inline const juce::Colour accentLight     {0xff9d5cfa};
        static inline const juce::Colour gold            {0xffffd700};
        static inline const juce::Colour goldDark        {0xffb8960f};
        static inline const juce::Colour textPrimary     {0xffe0e0e0};
        static inline const juce::Colour textSecondary   {0xff8a8aa0};
        static inline const juce::Colour toggleOn        {0xff00e676};
        static inline const juce::Colour toggleOff       {0xff555570};
        static inline const juce::Colour danger          {0xffff5252};
    };

    SolfeggioLookAndFeel() {
        setColour(juce::ResizableWindow::backgroundColourId, Colors::background);
        setColour(juce::Slider::rotarySliderFillColourId, Colors::accent);
        setColour(juce::Slider::rotarySliderOutlineColourId, Colors::surface);
        setColour(juce::Slider::thumbColourId, Colors::gold);
        setColour(juce::Label::textColourId, Colors::textPrimary);
        setColour(juce::TextButton::buttonColourId, Colors::surface);
        setColour(juce::TextButton::textColourOnId, Colors::textPrimary);
    }

    //==========================================================================
    // Custom rotary slider drawing
    //==========================================================================
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& /*slider*/) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto diameter = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Outer glow
        auto glowColor = Colors::accent.withAlpha(0.15f);
        g.setColour(glowColor);
        g.fillEllipse(rx - 3, ry - 3, diameter + 6, diameter + 6);

        // Background arc (track)
        juce::Path bgArc;
        bgArc.addCentredArc(centreX, centreY, radius * 0.85f, radius * 0.85f,
                            0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(Colors::surface);
        g.strokePath(bgArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

        // Value arc with gradient
        if (sliderPos > 0.0f) {
            juce::Path valueArc;
            valueArc.addCentredArc(centreX, centreY, radius * 0.85f, radius * 0.85f,
                                   0.0f, rotaryStartAngle, angle, true);

            auto gradient = juce::ColourGradient(
                Colors::accent, centreX, centreY - radius,
                Colors::gold, centreX, centreY + radius, false);
            g.setGradientFill(gradient);
            g.strokePath(valueArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
        }

        // Inner circle (knob body)
        auto knobRadius = radius * 0.65f;
        auto knobGradient = juce::ColourGradient(
            Colors::surfaceLight, centreX, centreY - knobRadius,
            Colors::background, centreX, centreY + knobRadius, false);
        g.setGradientFill(knobGradient);
        g.fillEllipse(centreX - knobRadius, centreY - knobRadius,
                      knobRadius * 2.0f, knobRadius * 2.0f);

        // Knob border
        g.setColour(Colors::surface.brighter(0.3f));
        g.drawEllipse(centreX - knobRadius, centreY - knobRadius,
                      knobRadius * 2.0f, knobRadius * 2.0f, 1.5f);

        // Pointer line
        auto pointerLength = knobRadius * 0.7f;
        auto pointerThickness = 2.5f;
        juce::Path pointer;
        pointer.addRoundedRectangle(-pointerThickness * 0.5f, -knobRadius + 4.0f,
                                     pointerThickness, pointerLength, 1.0f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle)
                                   .translated(centreX, centreY));
        g.setColour(Colors::gold);
        g.fillPath(pointer);

        // Center dot
        g.setColour(Colors::gold.withAlpha(0.6f));
        g.fillEllipse(centreX - 2.5f, centreY - 2.5f, 5.0f, 5.0f);
    }

    //==========================================================================
    // Custom toggle button
    //==========================================================================
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 4.0f;
        auto r = juce::Rectangle<float>(bounds.getX() + (bounds.getWidth() - size) * 0.5f,
                                         bounds.getY() + (bounds.getHeight() - size) * 0.5f,
                                         size, size);

        bool isOn = button.getToggleState();

        // Background circle
        auto bgColor = isOn ? Colors::toggleOn.withAlpha(0.2f) : Colors::toggleOff.withAlpha(0.3f);
        g.setColour(bgColor);
        g.fillEllipse(r);

        // Border
        auto borderColor = isOn ? Colors::toggleOn : Colors::toggleOff;
        if (shouldDrawButtonAsHighlighted)
            borderColor = borderColor.brighter(0.3f);
        g.setColour(borderColor);
        g.drawEllipse(r, 2.0f);

        // Inner filled circle when on
        if (isOn) {
            auto innerR = r.reduced(size * 0.25f);
            g.setColour(Colors::toggleOn);
            g.fillEllipse(innerR);
        }

        juce::ignoreUnused(shouldDrawButtonAsDown);
    }

    //==========================================================================
    // Custom linear slider (for master mix)
    //==========================================================================
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearHorizontal) {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                              minSliderPos, maxSliderPos, style, slider);
            return;
        }

        auto bounds = juce::Rectangle<float>(
            static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(width), static_cast<float>(height));

        auto trackY = bounds.getCentreY();
        auto trackHeight = 6.0f;
        auto trackBounds = juce::Rectangle<float>(
            bounds.getX() + 4, trackY - trackHeight * 0.5f,
            bounds.getWidth() - 8, trackHeight);

        // Track background
        g.setColour(Colors::surface);
        g.fillRoundedRectangle(trackBounds, trackHeight * 0.5f);

        // Filled portion
        auto filledWidth = sliderPos - trackBounds.getX();
        if (filledWidth > 0) {
            auto filledBounds = trackBounds.withWidth(filledWidth);
            auto gradient = juce::ColourGradient(
                Colors::accent, filledBounds.getX(), trackY,
                Colors::gold, filledBounds.getRight(), trackY, false);
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(filledBounds, trackHeight * 0.5f);
        }

        // Thumb
        auto thumbRadius = 8.0f;
        g.setColour(Colors::gold);
        g.fillEllipse(sliderPos - thumbRadius, trackY - thumbRadius,
                      thumbRadius * 2.0f, thumbRadius * 2.0f);

        // Thumb glow
        g.setColour(Colors::gold.withAlpha(0.2f));
        g.fillEllipse(sliderPos - thumbRadius - 3, trackY - thumbRadius - 3,
                      (thumbRadius + 3) * 2.0f, (thumbRadius + 3) * 2.0f);

        juce::ignoreUnused(minSliderPos, maxSliderPos);
    }

    //==========================================================================
    // Font
    //==========================================================================
    juce::Font getLabelFont(juce::Label&) override {
        return juce::Font(13.0f);
    }
};
