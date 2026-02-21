#pragma once
#include <JuceHeader.h>
#include <array>
#include <string>

// ============================================================================
// Solfeggio namespace — project-wide constants, parameter IDs, and metadata.
// Centralizing here means one place to update if frequencies or IDs change.
// ============================================================================
namespace Solfeggio {

    inline constexpr int NUM_FREQUENCIES = 10;

    inline constexpr std::array<float, NUM_FREQUENCIES> Frequencies = {
        174.0f, 285.0f, 396.0f, 417.0f, 432.0f,
        528.0f, 639.0f, 741.0f, 852.0f, 963.0f
    };

    inline const std::array<const char*, NUM_FREQUENCIES> FrequencyNames = {
        "174 Hz", "285 Hz", "396 Hz", "417 Hz", "432 Hz",
        "528 Hz", "639 Hz", "741 Hz", "852 Hz", "963 Hz"
    };

    inline const std::array<const char*, NUM_FREQUENCIES> FrequencyDescriptions = {
        "Pain Relief",  "Tissue Healing", "Liberation",   "Change",       "Natural Tuning",
        "Transformation","Connection",    "Expression",   "Intuition",    "Enlightenment"
    };

    // -------------------------------------------------------------------------
    // Parameter IDs — centralised so no magic strings exist in processor code
    // -------------------------------------------------------------------------
    namespace Params {
        inline juce::ParameterID autoMode    { "autoMode",    1 };
        inline juce::ParameterID cycleTime   { "cycleTime",   1 };
        inline juce::ParameterID autoIntensity{ "autoIntensity",1 };
        inline juce::ParameterID masterMix   { "masterMix",   1 };
        inline juce::ParameterID scAttack    { "scAttack",    1 };
        inline juce::ParameterID scRelease   { "scRelease",   1 };
        inline juce::ParameterID scDryWet    { "scDryWet",    1 };

        inline juce::String getGainID(float freq) { return juce::String(freq, 0) + "Hz_Gain"; }
        inline juce::String getOnID  (float freq)  { return juce::String(freq, 0) + "Hz_On";   }
    }

    // -------------------------------------------------------------------------
    // UI layout constants — avoids magic numbers in resized() calls
    // -------------------------------------------------------------------------
    namespace Layout {
        inline constexpr int editorWidth  = 1200;
        inline constexpr int editorHeight = 800;
        inline constexpr int minWidth     = 900;
        inline constexpr int minHeight    = 600;
    }

} // namespace Solfeggio
