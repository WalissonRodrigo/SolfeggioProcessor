cmake_minimum_required(VERSION 3.16)
project(SolfeggioPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable SIMD optimizations (architecture-aware)
if(MSVC)
    add_compile_options(/arch:AVX2)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i[3-6]86")
    add_compile_options(-msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mavx -mavx2)
endif()
# ARM/Apple Silicon uses NEON automatically — no flags needed

# ============================================================================
# Fetch JUCE via FetchContent
# ============================================================================
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ============================================================================
# Plugin target
# ============================================================================
juce_add_plugin(SolfeggioPlugin
    PLUGIN_MANUFACTURER_CODE Solf
    PLUGIN_CODE               Sfgg
    FORMATS                   VST3 Standalone  # AU added below for macOS
    PRODUCT_NAME              "Solfeggio Frequencies"
    COMPANY_NAME              "Walisson Rodrigo"
    BUNDLE_ID                 "com.walissonrodrigo.solfeggio"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    PLUGIN_MANUFACTURER_URL   "https://github.com/WalissonRodrigo"
    PLUGIN_DESCRIPTION        "Subliminal Solfeggio frequency processing"
)

# Generate JuceHeader.h
juce_generate_juce_header(SolfeggioPlugin)

# Add AU format on macOS
if(APPLE)
    set_target_properties(SolfeggioPlugin PROPERTIES
        JUCE_FORMATS "VST3;AU;Standalone"
    )
endif()

# ============================================================================
# Source files
# ============================================================================
target_sources(SolfeggioPlugin PRIVATE
    Source/SolfeggioProcessor.cpp
    Source/PluginEditor.cpp
)

# Headers (for IDE visibility)
target_sources(SolfeggioPlugin PRIVATE
    Source/SolfeggioProcessor.h
    Source/PluginEditor.h
    Source/LookAndFeel.h
    Source/SpectrumAnalyzer.h
)

# ============================================================================
# Compile definitions
# ============================================================================
target_compile_definitions(SolfeggioPlugin PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

# ============================================================================
# Link JUCE modules
# ============================================================================
target_link_libraries(SolfeggioPlugin PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ============================================================================
# macOS Codesigning
# ============================================================================
if(APPLE)
    # Ad-hoc signing by default, override with -DCODESIGN_IDENTITY="Developer ID"
    set(CODESIGN_IDENTITY "-" CACHE STRING "Code signing identity")
    set_target_properties(SolfeggioPlugin PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${CODESIGN_IDENTITY}"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
    )
endif()

# ============================================================================
# CPack installers
# ============================================================================
set(CPACK_PACKAGE_NAME "SolfeggioFrequencies")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "Walisson Rodrigo")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Subliminal Solfeggio Frequency Audio Plugin")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Solfeggio Frequencies")
    set(CPACK_NSIS_PACKAGE_NAME "SolfeggioFrequencies")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/Resources/icon.ico")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64\\\\VST3")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "Solfeggio Frequencies")
endif()

include(CPack)