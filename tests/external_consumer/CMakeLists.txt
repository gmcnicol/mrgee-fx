cmake_minimum_required(VERSION 3.24)

project(mrgee_fx_external_consumer_smoke VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED MRGEE_FX_SOURCE_DIR)
    get_filename_component(MRGEE_FX_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

set(MRGEE_BUILD_EXAMPLES OFF CACHE BOOL "Do not build mrgee-fx examples from consumer smoke" FORCE)
set(MRGEE_USE_YSFX OFF CACHE BOOL "Keep external consumer smoke configure/build small" FORCE)

add_subdirectory("${MRGEE_FX_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_deps/mrgee-fx")

if(TARGET MrgeeSwitchableFilter)
    message(FATAL_ERROR "mrgee-fx examples should not build when imported by an external consumer")
endif()

mrgee_add_jsfx_plugin(
    TARGET ExternalConsumerDelay
    PRODUCT_NAME "External Consumer Delay"
    PLUGIN_CODE EcD1
    JSFX_FILE "${CMAKE_CURRENT_LIST_DIR}/jsfx/ConsumerDelay.jsfx"
    JSFX_ASSETS
        "${CMAKE_CURRENT_LIST_DIR}/jsfx/lib"
    FORMATS Standalone
)

