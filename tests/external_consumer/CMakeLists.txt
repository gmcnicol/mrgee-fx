cmake_minimum_required(VERSION 3.24)

project(mrgee_fx_external_consumer_smoke VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

if(NOT DEFINED MRGEE_FX_SOURCE_DIR)
    get_filename_component(MRGEE_FX_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

set(MRGEE_BUILD_EXAMPLES OFF CACHE BOOL "Do not build mrgee-fx examples from consumer smoke" FORCE)
set(YSFX_PLUGIN OFF CACHE BOOL "" FORCE)
set(YSFX_TESTS OFF CACHE BOOL "" FORCE)
set(YSFX_TOOLS OFF CACHE BOOL "" FORCE)
set(YSFX_SKIP_CHECKSUM ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    ysfx
    GIT_REPOSITORY https://github.com/jpcima/ysfx.git
    GIT_TAG 8077347ccf4115567aed81400281dca57acbb0cc
    GIT_SUBMODULES_RECURSE TRUE
)

FetchContent_MakeAvailable(ysfx)

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
