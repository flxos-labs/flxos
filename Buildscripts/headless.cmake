#
# Resolve FlxOS headless mode in both normal CMake configure and ESP-IDF's
# early "component_get_requirements" expansion pass.
#
function(flx_resolve_headless out_var)
    set(_headless OFF)

    # Preferred signal from top-level CMake.
    if(DEFINED FLXOS_HEADLESS_MODE_ENABLED)
        if(FLXOS_HEADLESS_MODE_ENABLED)
            set(_headless ON)
        endif()
    # Fallback signal if Kconfig value is already available in this scope.
    elseif(DEFINED CONFIG_FLXOS_HEADLESS_MODE)
        if(CONFIG_FLXOS_HEADLESS_MODE)
            set(_headless ON)
        endif()
    else()
        # Early expansion fallback: inspect sdkconfig directly.
        set(_scan_files
            "${CMAKE_SOURCE_DIR}/sdkconfig"
            "${CMAKE_SOURCE_DIR}/sdkconfig.defaults"
        )

        # Also scan sdkconfig.defaults.{IDF_TARGET} (e.g. sdkconfig.defaults.esp32s3).
        if(DEFINED IDF_TARGET AND NOT IDF_TARGET STREQUAL "")
            list(APPEND _scan_files "${CMAKE_SOURCE_DIR}/sdkconfig.defaults.${IDF_TARGET}")
        endif()

        if(DEFINED SDKCONFIG)
            list(APPEND _scan_files "${SDKCONFIG}")
        endif()

        if(DEFINED SDKCONFIG_DEFAULTS)
            foreach(_defaults_file IN LISTS SDKCONFIG_DEFAULTS)
                if(IS_ABSOLUTE "${_defaults_file}")
                    list(APPEND _scan_files "${_defaults_file}")
                else()
                    list(APPEND _scan_files "${CMAKE_SOURCE_DIR}/${_defaults_file}")
                endif()
            endforeach()
        endif()

        list(REMOVE_DUPLICATES _scan_files)
        foreach(_scan_file IN LISTS _scan_files)
            if(EXISTS "${_scan_file}")
                file(STRINGS "${_scan_file}" _headless_lines REGEX "^CONFIG_FLXOS_HEADLESS_MODE=y$")
                if(_headless_lines)
                    set(_headless ON)
                    break()
                endif()
            endif()
        endforeach()
    endif()

    set(${out_var} ${_headless} PARENT_SCOPE)
endfunction()
