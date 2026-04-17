cmake_minimum_required(VERSION 3.24)

project(mrgee_fx_bootstrap VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(MRGEE_FETCH_JUCE "Fetch JUCE automatically when third_party/JUCE is missing" ON)
option(MRGEE_USE_YSFX "Enable ysfx runtime integration" ON)

include(cmake/Dependencies.cmake)

juce_add_plugin(MrgeeJSFXBridge
    COMPANY_NAME "Mrgee"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE MrgE
    PLUGIN_CODE JsB1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Mrgee JSFX Bridge"
)

target_sources(MrgeeJSFXBridge
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginProcessor.h
        src/PluginEditor.cpp
        src/PluginEditor.h
        src/JsfxHost.h
)

target_compile_definitions(MrgeeJSFXBridge
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

if(MRGEE_YSFX_ENABLED)
    target_compile_definitions(MrgeeJSFXBridge PRIVATE MRGEE_HAS_YSFX=1)
endif()

target_link_libraries(MrgeeJSFXBridge
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)


if(TARGET ysfx)
    target_link_libraries(MrgeeJSFXBridge PRIVATE ysfx)
endif()
