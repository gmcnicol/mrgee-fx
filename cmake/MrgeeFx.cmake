include_guard(GLOBAL)

set(MRGEE_FX_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." CACHE INTERNAL "mrgee-fx source root")

function(_mrgee_resolve_path OUT_PATH INPUT_PATH)
    if(IS_ABSOLUTE "${INPUT_PATH}")
        set(resolved "${INPUT_PATH}")
    else()
        set(resolved "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_PATH}")
    endif()

    get_filename_component(resolved "${resolved}" ABSOLUTE)
    set(${OUT_PATH} "${resolved}" PARENT_SCOPE)
endfunction()

function(_mrgee_make_cpp_string OUT_VALUE INPUT_VALUE)
    set(value "${INPUT_VALUE}")
    string(REPLACE "\\" "\\\\" value "${value}")
    string(REPLACE "\"" "\\\"" value "${value}")
    set(${OUT_VALUE} "\"${value}\"" PARENT_SCOPE)
endfunction()

function(_mrgee_logical_asset_path OUT_PATH SCRIPT_DIR ASSET_ROOT ASSET_FILE)
    file(RELATIVE_PATH script_relative "${SCRIPT_DIR}" "${ASSET_FILE}")

    if(NOT script_relative MATCHES "^\\.\\." AND NOT IS_ABSOLUTE "${script_relative}")
        set(logical_path "${script_relative}")
    elseif(IS_DIRECTORY "${ASSET_ROOT}")
        get_filename_component(asset_root_parent "${ASSET_ROOT}" DIRECTORY)
        file(RELATIVE_PATH logical_path "${asset_root_parent}" "${ASSET_FILE}")
    else()
        get_filename_component(logical_path "${ASSET_FILE}" NAME)
    endif()

    file(TO_CMAKE_PATH "${logical_path}" logical_path)
    set(${OUT_PATH} "${logical_path}" PARENT_SCOPE)
endfunction()

function(mrgee_add_jsfx_plugin)
    set(options
        IS_SYNTH
        NEEDS_MIDI_INPUT
        NEEDS_MIDI_OUTPUT
        IS_MIDI_EFFECT
    )
    set(one_value_args
        TARGET
        PRODUCT_NAME
        PLUGIN_CODE
        JSFX_FILE
        COMPANY_NAME
        PLUGIN_MANUFACTURER_CODE
    )
    set(multi_value_args
        JSFX_ASSETS
        FORMATS
    )
    cmake_parse_arguments(MRGEE_PLUGIN "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    foreach(required_arg IN ITEMS TARGET PRODUCT_NAME PLUGIN_CODE JSFX_FILE)
        if(NOT MRGEE_PLUGIN_${required_arg})
            message(FATAL_ERROR "mrgee_add_jsfx_plugin requires ${required_arg}")
        endif()
    endforeach()

    foreach(flag IN ITEMS IS_SYNTH NEEDS_MIDI_INPUT NEEDS_MIDI_OUTPUT IS_MIDI_EFFECT)
        if(NOT DEFINED MRGEE_PLUGIN_${flag})
            set(MRGEE_PLUGIN_${flag} FALSE)
        endif()
    endforeach()

    if(NOT MRGEE_PLUGIN_COMPANY_NAME)
        set(MRGEE_PLUGIN_COMPANY_NAME "Mrgee")
    endif()

    if(NOT MRGEE_PLUGIN_PLUGIN_MANUFACTURER_CODE)
        set(MRGEE_PLUGIN_PLUGIN_MANUFACTURER_CODE MrgE)
    endif()

    if(NOT MRGEE_PLUGIN_FORMATS)
        set(MRGEE_PLUGIN_FORMATS VST3 AU Standalone)
    endif()

    _mrgee_resolve_path(jsfx_file "${MRGEE_PLUGIN_JSFX_FILE}")
    if(NOT EXISTS "${jsfx_file}")
        message(FATAL_ERROR "JSFX_FILE does not exist: ${jsfx_file}")
    endif()

    get_filename_component(jsfx_dir "${jsfx_file}" DIRECTORY)
    get_filename_component(jsfx_name "${jsfx_file}" NAME)

    set(source_files "${jsfx_file}")
    set(logical_paths "${jsfx_name}")

    foreach(asset IN LISTS MRGEE_PLUGIN_JSFX_ASSETS)
        _mrgee_resolve_path(asset_path "${asset}")
        if(IS_DIRECTORY "${asset_path}")
            file(GLOB_RECURSE asset_files CONFIGURE_DEPENDS LIST_DIRECTORIES FALSE "${asset_path}/*")
            list(SORT asset_files)
            foreach(asset_file IN LISTS asset_files)
                _mrgee_logical_asset_path(logical_path "${jsfx_dir}" "${asset_path}" "${asset_file}")
                list(APPEND source_files "${asset_file}")
                list(APPEND logical_paths "${logical_path}")
            endforeach()
        elseif(EXISTS "${asset_path}")
            _mrgee_logical_asset_path(logical_path "${jsfx_dir}" "${asset_path}" "${asset_path}")
            list(APPEND source_files "${asset_path}")
            list(APPEND logical_paths "${logical_path}")
        else()
            message(FATAL_ERROR "JSFX_ASSETS entry does not exist: ${asset_path}")
        endif()
    endforeach()

    list(LENGTH source_files source_count)
    math(EXPR last_index "${source_count} - 1")

    set(bundle_dir "${CMAKE_CURRENT_BINARY_DIR}/mrgee_jsfx_bundles/${MRGEE_PLUGIN_TARGET}")
    file(MAKE_DIRECTORY "${bundle_dir}/staged")

    set(staged_sources "")
    set(cpp_logical_paths "")
    foreach(index RANGE 0 ${last_index})
        list(GET source_files ${index} source_file)
        list(GET logical_paths ${index} logical_path)
        string(MAKE_C_IDENTIFIER "${logical_path}" safe_name)
        set(staged_file "${bundle_dir}/staged/resource_${index}_${safe_name}.bin")
        configure_file("${source_file}" "${staged_file}" COPYONLY)
        list(APPEND staged_sources "${staged_file}")

        _mrgee_make_cpp_string(cpp_logical_path "${logical_path}")
        string(APPEND cpp_logical_paths "    ${cpp_logical_path},\n")
    endforeach()

    _mrgee_make_cpp_string(cpp_main_script_name "${jsfx_name}")
    _mrgee_make_cpp_string(cpp_bundle_id "${MRGEE_PLUGIN_TARGET}")

    set(generated_header "${bundle_dir}/MrgeeJsfxBundle.h")
    configure_file("${MRGEE_FX_ROOT}/cmake/MrgeeJsfxBundle.h.in" "${generated_header}" @ONLY)

    set(asset_target "${MRGEE_PLUGIN_TARGET}Assets")
    juce_add_binary_data(${asset_target}
        SOURCES
            ${staged_sources}
    )

    juce_add_plugin(${MRGEE_PLUGIN_TARGET}
        COMPANY_NAME "${MRGEE_PLUGIN_COMPANY_NAME}"
        IS_SYNTH ${MRGEE_PLUGIN_IS_SYNTH}
        NEEDS_MIDI_INPUT ${MRGEE_PLUGIN_NEEDS_MIDI_INPUT}
        NEEDS_MIDI_OUTPUT ${MRGEE_PLUGIN_NEEDS_MIDI_OUTPUT}
        IS_MIDI_EFFECT ${MRGEE_PLUGIN_IS_MIDI_EFFECT}
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE ${MRGEE_PLUGIN_PLUGIN_MANUFACTURER_CODE}
        PLUGIN_CODE ${MRGEE_PLUGIN_PLUGIN_CODE}
        FORMATS ${MRGEE_PLUGIN_FORMATS}
        PRODUCT_NAME "${MRGEE_PLUGIN_PRODUCT_NAME}"
    )

    target_sources(${MRGEE_PLUGIN_TARGET}
        PRIVATE
            "${MRGEE_FX_ROOT}/src/PluginProcessor.cpp"
            "${MRGEE_FX_ROOT}/src/PluginProcessor.h"
            "${MRGEE_FX_ROOT}/src/PluginEditor.cpp"
            "${MRGEE_FX_ROOT}/src/PluginEditor.h"
            "${MRGEE_FX_ROOT}/src/JsfxHost.cpp"
            "${MRGEE_FX_ROOT}/src/JsfxHost.h"
            "${MRGEE_FX_ROOT}/src/YsfxMidiBridge.cpp"
            "${MRGEE_FX_ROOT}/src/YsfxMidiBridge.h"
            "${generated_header}"
    )

    target_include_directories(${MRGEE_PLUGIN_TARGET}
        PRIVATE
            "${bundle_dir}"
            "${MRGEE_FX_ROOT}/src"
    )

    target_compile_definitions(${MRGEE_PLUGIN_TARGET}
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
    )

    if(MRGEE_YSFX_ENABLED)
        target_compile_definitions(${MRGEE_PLUGIN_TARGET} PRIVATE MRGEE_HAS_YSFX=1)
    endif()

    target_link_libraries(${MRGEE_PLUGIN_TARGET}
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            ${asset_target}
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )

    if(TARGET ysfx)
        target_link_libraries(${MRGEE_PLUGIN_TARGET} PRIVATE ysfx)
    endif()
endfunction()
