cmake_minimum_required(VERSION 3.16)
project(SolfeggioPlugin VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# SIMD optimizations (architecture-aware)
# ARM/Apple Silicon uses NEON automatically — no flags needed
# ============================================================================
if(MSVC)
    add_compile_options(/arch:AVX2)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i[3-6]86")
    add_compile_options(-msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mavx -mavx2)
endif()

include(CMake/EmbedIcon.cmake)

# ============================================================================
# Fetch JUCE via FetchContent
# ============================================================================
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# ============================================================================
# Plugin target
# ============================================================================
juce_add_plugin(SolfeggioPlugin
    PLUGIN_MANUFACTURER_CODE Solf
    PLUGIN_CODE Sfgg
    FORMATS VST3 Standalone
    PRODUCT_NAME "Solfeggio Frequencies"
    COMPANY_NAME "WR Systems (Walisson Rodrigo)"
    BUNDLE_ID "com.walissonrodrigo.solfeggio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    ICON_BIG "${CMAKE_SOURCE_DIR}/Resources/icon_256.png"
    ICON_SMALL "${CMAKE_SOURCE_DIR}/Resources/icon_32.png"
    PLUGIN_MANUFACTURER_URL "https://github.com/WalissonRodrigo/SolfeggioProcessor"
    PLUGIN_DESCRIPTION "Subliminal Solfeggio frequency processing"
)

# Embed icon as binary data for cross-platform robustness
embed_icon(SolfeggioPlugin "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon_256.png")

# Native Windows Icon Resource (for .exe taskbar icon — Standalone only)
# VST3 is a DLL and does NOT support .rc icon resources.
# Adding it to both targets causes a CVT1100 duplicate resource linker error.
if(WIN32)
    set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon.ico")
    if(EXISTS ${APP_ICON})
        set(ICON_RC "${CMAKE_BINARY_DIR}/icon.rc")
        file(WRITE ${ICON_RC} "IDI_ICON1 ICON DISCARDABLE \"${APP_ICON}\"\n")
        target_sources(SolfeggioPlugin_Standalone PRIVATE ${ICON_RC})
        set_target_properties(SolfeggioPlugin_Standalone PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()

# Generate JuceHeader.h
juce_generate_juce_header(SolfeggioPlugin)

# Add AU format on macOS
if(APPLE)
    set_target_properties(SolfeggioPlugin PROPERTIES
        JUCE_FORMATS "VST3;AU;Standalone"
    )
endif()

# ============================================================================
# Source files — organized by architectural layer
# ============================================================================
target_sources(SolfeggioPlugin PRIVATE
    # Plugin layer (JUCE entry-point / Controller)
    Source/Plugin/SolfeggioProcessor.cpp

    # DSP layer (Model / Audio engine)
    Source/DSP/SolfeggioEngine.cpp
    Source/DSP/SmartAutoEngine.cpp
    Source/DSP/SidechainCompressor.cpp

    # GUI layer (View)
    Source/GUI/PluginEditor.cpp

    # Headers — listed for IDE visibility
    Source/Plugin/SolfeggioProcessor.h
    Source/DSP/SolfeggioEngine.h
    Source/DSP/SmartAutoEngine.h
    Source/DSP/SidechainCompressor.h
    Source/GUI/PluginEditor.h
    Source/GUI/SpectrumAnalyzer.h
    Source/GUI/AutoModeBar.h
    Source/GUI/FrequencyGrid.h
    Source/GUI/FrequencyControl.h
    Source/Core/Constants.h
    Source/Core/LookAndFeel.h
    Source/Core/WindowsIconHelpers.h
)

# ============================================================================
# Include directories — each layer is a search root so includes stay simple
# ============================================================================
target_include_directories(SolfeggioPlugin PRIVATE
    Source/Core
    Source/DSP
    Source/GUI
    Source/Plugin
)

# ============================================================================
# Compile definitions
# ============================================================================
target_compile_definitions(SolfeggioPlugin PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
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
