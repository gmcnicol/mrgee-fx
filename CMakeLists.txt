cmake_minimum_required(VERSION 3.24)

project(mrgee_fx VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(MRGEE_FETCH_JUCE "Fetch JUCE automatically when third_party/JUCE is missing" ON)
option(MRGEE_USE_YSFX "Enable ysfx runtime integration" ON)
option(MRGEE_BUILD_EXAMPLES "Build mrgee-fx examples and smoke tools" ${PROJECT_IS_TOP_LEVEL})

include(cmake/Dependencies.cmake)
include(cmake/MrgeeFx.cmake)

if(NOT MRGEE_BUILD_EXAMPLES)
    return()
endif()

set(MRGEE_PRIMARY_PLUGIN_TARGET MrgeeSwitchableFilter)
set(MRGEE_PRIMARY_PLUGIN_PRODUCT_NAME "Mrgee Switchable Filter")
set(MRGEE_PRIMARY_PLUGIN_VST3_PATH
    "${CMAKE_BINARY_DIR}/${MRGEE_PRIMARY_PLUGIN_TARGET}_artefacts/$<CONFIG>/VST3/${MRGEE_PRIMARY_PLUGIN_PRODUCT_NAME}.vst3")

mrgee_add_jsfx_plugin(
    TARGET ${MRGEE_PRIMARY_PLUGIN_TARGET}
    PRODUCT_NAME "${MRGEE_PRIMARY_PLUGIN_PRODUCT_NAME}"
    PLUGIN_CODE SwF1
    JSFX_FILE assets/jsfx/SwitchableFilter.jsfx
)

mrgee_add_jsfx_plugin(
    TARGET MrgeeAssetSmoke
    PRODUCT_NAME "Mrgee Asset Smoke"
    PLUGIN_CODE AsS1
    JSFX_FILE assets/jsfx/asset_smoke/AssetSmoke.jsfx
    JSFX_ASSETS
        assets/jsfx/asset_smoke/lib
        assets/jsfx/asset_smoke/Data
    FORMATS Standalone
)

juce_add_console_app(mrgee_jsfx_smoke PRODUCT_NAME "mrgee_jsfx_smoke")

target_sources(mrgee_jsfx_smoke
    PRIVATE
        tools/jsfx_smoke.cpp
)

target_compile_definitions(mrgee_jsfx_smoke
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_PLUGINHOST_VST3=1
)

if(MRGEE_YSFX_ENABLED)
    target_compile_definitions(mrgee_jsfx_smoke PRIVATE MRGEE_HAS_YSFX=1)
endif()

target_link_libraries(mrgee_jsfx_smoke
    PRIVATE
        ${MRGEE_PRIMARY_PLUGIN_TARGET}
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

juce_add_console_app(mrgee_jsfx_asset_smoke PRODUCT_NAME "mrgee_jsfx_asset_smoke")

target_sources(mrgee_jsfx_asset_smoke
    PRIVATE
        tools/jsfx_asset_smoke.cpp
)

target_compile_definitions(mrgee_jsfx_asset_smoke
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

if(MRGEE_YSFX_ENABLED)
    target_compile_definitions(mrgee_jsfx_asset_smoke PRIVATE MRGEE_HAS_YSFX=1)
endif()

target_link_libraries(mrgee_jsfx_asset_smoke
    PRIVATE
        MrgeeAssetSmoke
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(MRGEE_YSFX_ENABLED)
    juce_add_console_app(mrgee_midi_bridge_smoke PRODUCT_NAME "mrgee_midi_bridge_smoke")

    target_sources(mrgee_midi_bridge_smoke
        PRIVATE
            tools/midi_bridge_smoke.cpp
            src/YsfxMidiBridge.cpp
            src/YsfxMidiBridge.h
    )

    target_compile_definitions(mrgee_midi_bridge_smoke
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            MRGEE_HAS_YSFX=1
    )

    target_link_libraries(mrgee_midi_bridge_smoke
        PRIVATE
            juce::juce_audio_basics
            ysfx
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )
endif()

juce_add_console_app(mrgee_vst3_smoke PRODUCT_NAME "mrgee_vst3_smoke")

target_sources(mrgee_vst3_smoke
    PRIVATE
        tools/vst3_smoke.cpp
)

target_compile_definitions(mrgee_vst3_smoke
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_PLUGINHOST_VST3=1
)

target_link_libraries(mrgee_vst3_smoke
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
