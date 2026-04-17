include(FetchContent)

# JUCE
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/JUCE/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/JUCE" "${CMAKE_BINARY_DIR}/JUCE")
elseif(MRGEE_FETCH_JUCE)
    message(STATUS "JUCE not found in third_party/JUCE, fetching from GitHub...")
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
    )
    FetchContent_MakeAvailable(juce)
else()
    message(FATAL_ERROR "JUCE not found. Set MRGEE_FETCH_JUCE=ON or add JUCE to third_party/JUCE")
endif()

# ysfx
set(MRGEE_YSFX_ENABLED OFF)
if(MRGEE_USE_YSFX)
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/ysfx/CMakeLists.txt")
        add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/ysfx" "${CMAKE_BINARY_DIR}/ysfx")
        set(MRGEE_YSFX_ENABLED ON)
        if(TARGET ysfx)
            message(STATUS "ysfx target found and enabled")
        else()
            message(WARNING "ysfx source found but no 'ysfx' target exposed; build will compile without direct linking")
        endif()
    else()
        message(WARNING "ysfx not found in third_party/ysfx. Building with mock JSFX host. See README for setup.")
    endif()
endif()
