# ============================================================================
# FlxOS Profile Engine
# ============================================================================
# Replaces device.cmake + headless.cmake with a unified YAML-driven engine.
#
# Functions:
#   flx_load_profile()              — Top-level: parse YAML, generate Config.hpp + sdkconfig
#   _flx_parse_yaml(file result)    — Flat key=value parser for simple YAML
#   _flx_resolve_inheritance()      — Deep-merge base template with child profile
#   _flx_generate_config_hpp()      — Emit constexpr Config.hpp from parsed YAML data
#   _flx_generate_sdkconfig_frag()  — Emit sdkconfig.profile with LVGL/target/partition defaults
#   _flx_configure_headless()       — Set HEADLESS_MODE, EXCLUDE_COMPONENTS, LV_USE_LOVYAN_GFX
# ============================================================================

# --------------------------------------------------------------------------
# _flx_yaml_get(prefix key default out_var)
#   Retrieve a YAML value from the flat-parsed map. Falls back to default.
# --------------------------------------------------------------------------
macro(_flx_yaml_get PREFIX KEY DEFAULT OUT_VAR)
    if(DEFINED ${PREFIX}_${KEY})
        set(${OUT_VAR} "${${PREFIX}_${KEY}}")
    else()
        set(${OUT_VAR} "${DEFAULT}")
    endif()
endmacro()

# --------------------------------------------------------------------------
# _flx_parse_yaml(file prefix)
#   Reads a YAML file and sets flattened CMake variables:
#     <prefix>_<path> = <value>
#   e.g. hardware.display.width: 240 → <prefix>_hardware_display_width = "240"
#   Handles: scalars, inline sequences not supported (use one-key-per-line).
#   Limitation: No multi-line values, no anchors/aliases, no flow mappings.
# --------------------------------------------------------------------------
function(_flx_parse_yaml YAML_FILE PREFIX)
    if(NOT EXISTS "${YAML_FILE}")
        message(FATAL_ERROR "FlxOS: YAML file not found: ${YAML_FILE}")
    endif()

    file(STRINGS "${YAML_FILE}" _lines)

    # Stack of (indent_level, key_prefix) pairs — simulated with parallel lists
    # Note: _prefix_stack uses "_ROOT_" as sentinel because "" is treated as empty list by CMake
    set(_indent_stack "0")
    set(_prefix_stack "_ROOT_")

    foreach(_line IN LISTS _lines)
        # Skip comments and blank lines
        string(REGEX MATCH "^[ ]*#" _is_comment "${_line}")
        if(_is_comment)
            continue()
        endif()
        string(STRIP "${_line}" _stripped)
        if("${_stripped}" STREQUAL "")
            continue()
        endif()

        # Measure indent (number of leading spaces)
        # Use ( +) instead of ( *) to avoid CMake "matched an empty string" error
        string(REGEX MATCH "^( +)" _indent_match "${_line}")
        if(_indent_match)
            string(LENGTH "${_indent_match}" _indent)
        else()
            set(_indent 0)
        endif()

        # Pop stack entries whose indent >= current (they ended)
        list(LENGTH _indent_stack _stack_len)
        while(_stack_len GREATER 1)
            math(EXPR _top_idx "${_stack_len} - 1")
            list(GET _indent_stack ${_top_idx} _top_indent)
            if(_indent LESS_EQUAL _top_indent)
                list(REMOVE_AT _indent_stack ${_top_idx})
                list(REMOVE_AT _prefix_stack ${_top_idx})
                list(LENGTH _indent_stack _stack_len)
            else()
                break()
            endif()
        endwhile()

        # Build current prefix from stack (skip sentinel "_ROOT_")
        set(_current_prefix "")
        foreach(_p IN LISTS _prefix_stack)
            if(NOT "${_p}" STREQUAL "_ROOT_")
                if("${_current_prefix}" STREQUAL "")
                    set(_current_prefix "${_p}")
                else()
                    set(_current_prefix "${_current_prefix}_${_p}")
                endif()
            endif()
        endforeach()

        # Parse key: value
        if("${_stripped}" MATCHES "^([a-zA-Z0-9_-]+):[ ]*(.*)")
            set(_key "${CMAKE_MATCH_1}")
            set(_val "${CMAKE_MATCH_2}")

            # Normalize key: replace hyphens with underscores
            string(REPLACE "-" "_" _key "${_key}")

            # Remove inline comments
            string(REGEX REPLACE "[ ]+#.*$" "" _val "${_val}")
            # Strip surrounding quotes
            string(REGEX REPLACE "^\"(.*)\"$" "\\1" _val "${_val}")
            string(REGEX REPLACE "^'(.*)'$" "\\1" _val "${_val}")

            if("${_val}" STREQUAL "" OR "${_val}" MATCHES "^$")
                # This key introduces a nested map — push onto stack
                list(APPEND _indent_stack "${_indent}")
                list(APPEND _prefix_stack "${_key}")
            else()
                # Leaf value — set variable
                if("${_current_prefix}" STREQUAL "")
                    set(_full_key "${PREFIX}_${_key}")
                else()
                    set(_full_key "${PREFIX}_${_current_prefix}_${_key}")
                endif()
                set(${_full_key} "${_val}" PARENT_SCOPE)
            endif()
        elseif("${_stripped}" MATCHES "^- (.*)")
            # Array item — append to list variable (for tags, etc.)
            set(_item "${CMAKE_MATCH_1}")
            string(REGEX REPLACE "^\"(.*)\"$" "\\1" _item "${_item}")
            if("${_current_prefix}" STREQUAL "")
                set(_list_key "${PREFIX}__list")
            else()
                set(_list_key "${PREFIX}_${_current_prefix}")
            endif()
            # Append to list (local + parent scope so subsequent items are visible)
            if(DEFINED ${_list_key})
                set(_updated_list "${${_list_key}};${_item}")
            else()
                set(_updated_list "${_item}")
            endif()
            set(${_list_key} "${_updated_list}")
            set(${_list_key} "${_updated_list}" PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

# --------------------------------------------------------------------------
# _flx_resolve_inheritance(profile_dir prefix)
#   If the profile has 'inherits:', parse the base YAML first, then overlay.
# --------------------------------------------------------------------------
function(_flx_resolve_inheritance PROFILE_DIR PREFIX)
    set(_profile_yaml "${PROFILE_DIR}/profile.yaml")
    _flx_parse_yaml("${_profile_yaml}" "${PREFIX}")

    # Propagate all variables to parent scope
    get_cmake_property(_all_vars VARIABLES)
    foreach(_v IN LISTS _all_vars)
        if("${_v}" MATCHES "^${PREFIX}_")
            set(${_v} "${${_v}}" PARENT_SCOPE)
        endif()
    endforeach()

    # Check for inheritance
    _flx_yaml_get("${PREFIX}" "inherits" "" _inherits)
    if(NOT "${_inherits}" STREQUAL "" AND NOT "${_inherits}" STREQUAL "null")
        set(_base_yaml "${CMAKE_SOURCE_DIR}/Profiles/${_inherits}.yaml")
        if(EXISTS "${_base_yaml}")
            # Parse base into a temporary prefix
            _flx_parse_yaml("${_base_yaml}" "_BASE")
            # Set base values as defaults (don't overwrite child values)
            get_cmake_property(_base_vars VARIABLES)
            foreach(_bv IN LISTS _base_vars)
                if("${_bv}" MATCHES "^_BASE_(.*)")
                    set(_child_key "${PREFIX}_${CMAKE_MATCH_1}")
                    if(NOT DEFINED ${_child_key})
                        set(${_child_key} "${${_bv}}" PARENT_SCOPE)
                    endif()
                endif()
            endforeach()
        else()
            message(WARNING "FlxOS: Base profile not found: ${_base_yaml}")
        endif()
    endif()
endfunction()

# --------------------------------------------------------------------------
# _flx_spi_host_expr(host_num)
#   Convert YAML spi host number to C++ SPI_HOST enum expression.
# --------------------------------------------------------------------------
function(_flx_spi_host_expr HOST_NUM OUT_VAR)
    if("${HOST_NUM}" STREQUAL "1")
        set(${OUT_VAR} "SPI1_HOST" PARENT_SCOPE)
    elseif("${HOST_NUM}" STREQUAL "2")
        set(${OUT_VAR} "SPI2_HOST" PARENT_SCOPE)
    elseif("${HOST_NUM}" STREQUAL "3")
        set(${OUT_VAR} "SPI3_HOST" PARENT_SCOPE)
    elseif("${HOST_NUM}" STREQUAL "-1")
        set(${OUT_VAR} "static_cast<spi_host_device_t>(-1)" PARENT_SCOPE)
    else()
        set(${OUT_VAR} "SPI2_HOST" PARENT_SCOPE)
    endif()
endfunction()

# --------------------------------------------------------------------------
# _flx_bool(val)  — Normalize yaml bool to C++ true/false
# --------------------------------------------------------------------------
function(_flx_bool VAL OUT_VAR)
    string(TOLOWER "${VAL}" _lower)
    if("${_lower}" STREQUAL "true" OR "${_lower}" STREQUAL "yes" OR "${_lower}" STREQUAL "1")
        set(${OUT_VAR} "true" PARENT_SCOPE)
    else()
        set(${OUT_VAR} "false" PARENT_SCOPE)
    endif()
endfunction()

# --------------------------------------------------------------------------
# _flx_validate_profile(prefix)
#   Validate required fields and enum values before code generation.
# --------------------------------------------------------------------------
function(_flx_validate_profile PREFIX)
    foreach(_required IN ITEMS id vendor name target flash_size)
        if(NOT DEFINED ${PREFIX}_${_required})
            message(FATAL_ERROR "FlxOS: profile.yaml missing required field '${_required}'")
        endif()
        set(_required_val "${${PREFIX}_${_required}}")
        if("${_required_val}" STREQUAL "" OR "${_required_val}" STREQUAL "null")
            message(FATAL_ERROR "FlxOS: profile.yaml field '${_required}' cannot be empty/null")
        endif()
    endforeach()

    _flx_yaml_get("${PREFIX}" "target" "esp32" _target)
    set(_valid_targets esp32 esp32s3 esp32c6 esp32p4)
    list(FIND _valid_targets "${_target}" _target_idx)
    if(_target_idx EQUAL -1)
        message(FATAL_ERROR
            "FlxOS: Invalid target '${_target}'. Valid values: ${_valid_targets}")
    endif()

    _flx_yaml_get("${PREFIX}" "flash_size" "4MB" _flash_size)
    set(_valid_flash_sizes 4MB 8MB 16MB)
    list(FIND _valid_flash_sizes "${_flash_size}" _flash_idx)
    if(_flash_idx EQUAL -1)
        message(FATAL_ERROR
            "FlxOS: Invalid flash_size '${_flash_size}'. Valid values: ${_valid_flash_sizes}")
    endif()

    _flx_yaml_get("${PREFIX}" "flash_mode" "" _flash_mode)
    if(NOT "${_flash_mode}" STREQUAL "" AND NOT "${_flash_mode}" STREQUAL "null")
        string(TOUPPER "${_flash_mode}" _flash_mode_upper)
        set(_valid_flash_modes QIO DIO QOUT DOUT)
        list(FIND _valid_flash_modes "${_flash_mode_upper}" _flash_mode_idx)
        if(_flash_mode_idx EQUAL -1)
            message(FATAL_ERROR
                "FlxOS: Invalid flash_mode '${_flash_mode}'. Valid values: ${_valid_flash_modes}")
        endif()
    endif()

    _flx_yaml_get("${PREFIX}" "lvgl_ui_density" "normal" _ui_density)
    if("${_ui_density}" STREQUAL "" OR "${_ui_density}" STREQUAL "null")
        set(_ui_density "normal")
    endif()
    string(TOLOWER "${_ui_density}" _ui_density_lower)
    set(_valid_ui_density normal compact)
    list(FIND _valid_ui_density "${_ui_density_lower}" _ui_density_idx)
    if(_ui_density_idx EQUAL -1)
        message(FATAL_ERROR
            "FlxOS: Invalid lvgl.ui_density '${_ui_density}'. Valid values: ${_valid_ui_density}")
    endif()

    get_cmake_property(_all_vars VARIABLES)
    foreach(_var IN LISTS _all_vars)
        if("${_var}" MATCHES "^${PREFIX}_sdkconfig_(.+)")
            set(_sdk_key "${CMAKE_MATCH_1}")
            if(NOT "${_sdk_key}" MATCHES "^CONFIG_[A-Z0-9_]+$")
                message(FATAL_ERROR
                    "FlxOS: Invalid sdkconfig key '${_sdk_key}'. Keys must match ^CONFIG_[A-Z0-9_]+$")
            endif()
        endif()
    endforeach()
endfunction()

# --------------------------------------------------------------------------
# _flx_generate_config_hpp()
#   Generate modern C++ constexpr Config.hpp from parsed YAML variables.
# --------------------------------------------------------------------------
function(_flx_generate_config_hpp PREFIX OUTPUT_FILE)
    # Shortcuts
    macro(_y KEY DEFAULT OUT)
        _flx_yaml_get("${PREFIX}" "${KEY}" "${DEFAULT}" ${OUT})
    endmacro()
    macro(_b KEY DEFAULT OUT)
        _flx_yaml_get("${PREFIX}" "${KEY}" "${DEFAULT}" _raw_${OUT})
        _flx_bool("${_raw_${OUT}}" ${OUT})
    endmacro()

    # Profile metadata
    _y("id" "unknown" _id)
    _y("vendor" "Unknown" _vendor)
    _y("name" "Unknown Board" _name)
    set(_name_canonical "${_name}")
    if("${_name}" MATCHES ";")
        list(GET _name 0 _name_canonical)
    endif()
    _y("target" "esp32" _target)

    # Headless
    _b("headless" "false" _headless)

    # Display
    _b("hardware_display_enabled" "false" _disp_enabled)
    _y("hardware_display_driver" "None" _disp_driver)
    _y("hardware_display_width" "0" _disp_w)
    _y("hardware_display_height" "0" _disp_h)
    _y("hardware_display_rotation" "0" _disp_rot)
    _y("hardware_display_color_depth" "16" _disp_cdepth)
    _y("hardware_display_size_inches" "0.0" _disp_size)

    # Display SPI
    _y("hardware_display_bus" "spi" _disp_bus)
    _y("hardware_display_spi_host" "2" _disp_spi_host_num)
    _flx_spi_host_expr("${_disp_spi_host_num}" _disp_spi_host)
    _y("hardware_display_spi_mode" "0" _disp_spi_mode)
    _y("hardware_display_spi_freq_write" "40000000" _disp_spi_fw)
    _y("hardware_display_spi_freq_read" "16000000" _disp_spi_fr)
    _b("hardware_display_spi_three_wire" "false" _disp_spi_3w)
    _y("hardware_display_spi_dma_channel" "0" _disp_spi_dma)

    # Display I2C
    _y("hardware_display_i2c_port" "0" _disp_i2c_port)
    _y("hardware_display_i2c_addr" "0x3C" _disp_i2c_addr)
    _y("hardware_display_i2c_sda" "-1" _disp_i2c_sda)
    _y("hardware_display_i2c_scl" "-1" _disp_i2c_scl)
    _y("hardware_display_i2c_freq" "400000" _disp_i2c_freq)

    # Display Parallel
    _y("hardware_display_parallel_freq_write" "16000000" _disp_par_fw)
    _y("hardware_display_parallel_freq_read" "8000000" _disp_par_fr)
    _y("hardware_display_parallel_rd" "-1" _disp_par_rd)
    _y("hardware_display_parallel_wr" "-1" _disp_par_wr)
    _y("hardware_display_parallel_rs" "-1" _disp_par_rs)
    _y("hardware_display_parallel_d0" "-1" _disp_par_d0)
    _y("hardware_display_parallel_d1" "-1" _disp_par_d1)
    _y("hardware_display_parallel_d2" "-1" _disp_par_d2)
    _y("hardware_display_parallel_d3" "-1" _disp_par_d3)
    _y("hardware_display_parallel_d4" "-1" _disp_par_d4)
    _y("hardware_display_parallel_d5" "-1" _disp_par_d5)
    _y("hardware_display_parallel_d6" "-1" _disp_par_d6)
    _y("hardware_display_parallel_d7" "-1" _disp_par_d7)
    _y("hardware_display_parallel_d8" "-1" _disp_par_d8)
    _y("hardware_display_parallel_d9" "-1" _disp_par_d9)
    _y("hardware_display_parallel_d10" "-1" _disp_par_d10)
    _y("hardware_display_parallel_d11" "-1" _disp_par_d11)
    _y("hardware_display_parallel_d12" "-1" _disp_par_d12)
    _y("hardware_display_parallel_d13" "-1" _disp_par_d13)
    _y("hardware_display_parallel_d14" "-1" _disp_par_d14)
    _y("hardware_display_parallel_d15" "-1" _disp_par_d15)

    # Display RGB
    _y("hardware_display_rgb_freq_write" "16000000" _disp_rgb_fw)
    _y("hardware_display_rgb_pclk" "-1" _disp_rgb_pclk)
    _y("hardware_display_rgb_vsync" "-1" _disp_rgb_vsync)
    _y("hardware_display_rgb_hsync" "-1" _disp_rgb_hsync)
    _y("hardware_display_rgb_henable" "-1" _disp_rgb_henable)
    _y("hardware_display_rgb_d0" "-1" _disp_rgb_d0)
    _y("hardware_display_rgb_d1" "-1" _disp_rgb_d1)
    _y("hardware_display_rgb_d2" "-1" _disp_rgb_d2)
    _y("hardware_display_rgb_d3" "-1" _disp_rgb_d3)
    _y("hardware_display_rgb_d4" "-1" _disp_rgb_d4)
    _y("hardware_display_rgb_d5" "-1" _disp_rgb_d5)
    _y("hardware_display_rgb_d6" "-1" _disp_rgb_d6)
    _y("hardware_display_rgb_d7" "-1" _disp_rgb_d7)
    _y("hardware_display_rgb_d8" "-1" _disp_rgb_d8)
    _y("hardware_display_rgb_d9" "-1" _disp_rgb_d9)
    _y("hardware_display_rgb_d10" "-1" _disp_rgb_d10)
    _y("hardware_display_rgb_d11" "-1" _disp_rgb_d11)
    _y("hardware_display_rgb_d12" "-1" _disp_rgb_d12)
    _y("hardware_display_rgb_d13" "-1" _disp_rgb_d13)
    _y("hardware_display_rgb_d14" "-1" _disp_rgb_d14)
    _y("hardware_display_rgb_d15" "-1" _disp_rgb_d15)
    _y("hardware_display_rgb_hsync_pulse_width" "0" _disp_rgb_hpw)
    _y("hardware_display_rgb_hsync_back_porch" "0" _disp_rgb_hbp)
    _y("hardware_display_rgb_hsync_front_porch" "0" _disp_rgb_hfp)
    _y("hardware_display_rgb_vsync_pulse_width" "0" _disp_rgb_vpw)
    _y("hardware_display_rgb_vsync_back_porch" "0" _disp_rgb_vbp)
    _y("hardware_display_rgb_vsync_front_porch" "0" _disp_rgb_vfp)
    _b("hardware_display_rgb_hsync_polarity" "false" _disp_rgb_hp)
    _b("hardware_display_rgb_vsync_polarity" "false" _disp_rgb_vp)
    _b("hardware_display_rgb_pclk_active_neg" "true" _disp_rgb_pan)
    _b("hardware_display_rgb_de_idle_high" "false" _disp_rgb_deih)
    _b("hardware_display_rgb_pclk_idle_high" "false" _disp_rgb_pclkih)

    # Display pins
    _y("hardware_display_pins_cs" "-1" _pin_cs)
    _y("hardware_display_pins_dc" "-1" _pin_dc)
    _y("hardware_display_pins_rst" "-1" _pin_rst)
    _y("hardware_display_pins_busy" "-1" _pin_busy)
    _y("hardware_display_pins_bckl" "-1" _pin_bckl)
    _y("hardware_display_pins_mosi" "-1" _pin_mosi)
    _y("hardware_display_pins_sclk" "-1" _pin_sclk)
    _y("hardware_display_pins_miso" "-1" _pin_miso)

    # Panel
    _y("hardware_display_panel_offset_x" "0" _panel_ox)
    _y("hardware_display_panel_offset_y" "0" _panel_oy)
    _y("hardware_display_panel_offset_rotation" "0" _panel_or)
    _y("hardware_display_panel_dummy_read_pixel" "0" _panel_drp)
    _y("hardware_display_panel_dummy_read_bits" "0" _panel_drb)
    _b("hardware_display_panel_readable" "false" _panel_read)
    _b("hardware_display_panel_bus_shared" "false" _panel_bsh)
    _b("hardware_display_panel_invert" "false" _panel_inv)
    _b("hardware_display_panel_rgb_order" "false" _panel_rgb)
    _b("hardware_display_panel_dlen_16bit" "false" _panel_d16)

    # Backlight
    _y("hardware_display_backlight_freq" "1000" _bckl_freq)
    _y("hardware_display_backlight_pwm_channel" "0" _bckl_ch)
    _b("hardware_display_backlight_invert" "false" _bckl_inv)

    # Touch
    _b("hardware_touch_enabled" "false" _touch_en)
    _y("hardware_touch_driver" "None" _touch_drv)
    _y("hardware_touch_bus" "spi" _touch_bus)
    _y("hardware_touch_pins_cs" "-1" _tpin_cs)
    _y("hardware_touch_pins_int" "-1" _tpin_int)
    _y("hardware_touch_pins_sclk" "-1" _tpin_sclk)
    _y("hardware_touch_pins_mosi" "-1" _tpin_mosi)
    _y("hardware_touch_pins_miso" "-1" _tpin_miso)

    _y("hardware_touch_spi_host" "-1" _tspi_host_num)
    _flx_spi_host_expr("${_tspi_host_num}" _tspi_host)
    _y("hardware_touch_spi_freq" "1000000" _tspi_freq)
    _b("hardware_touch_spi_bus_shared" "false" _tspi_bsh)
    _b("hardware_touch_spi_separate_pins" "false" _tspi_sep)

    _y("hardware_touch_i2c_port" "0" _ti2c_port)
    _y("hardware_touch_i2c_addr" "0" _ti2c_addr)
    _y("hardware_touch_i2c_sda" "-1" _ti2c_sda)
    _y("hardware_touch_i2c_scl" "-1" _ti2c_scl)

    _y("hardware_touch_calibration_x_min" "0" _tcal_xmin)
    _y("hardware_touch_calibration_x_max" "0" _tcal_xmax)
    _y("hardware_touch_calibration_y_min" "0" _tcal_ymin)
    _y("hardware_touch_calibration_y_max" "0" _tcal_ymax)
    _y("hardware_touch_calibration_offset_rotation" "0" _tcal_or)

    # SD Card
    _b("hardware_sdcard_enabled" "false" _sd_en)
    _y("hardware_sdcard_cs" "-1" _sd_cs)
    _y("hardware_sdcard_spi_host" "2" _sd_spi_host_num)
    _flx_spi_host_expr("${_sd_spi_host_num}" _sd_spi_host)
    _y("hardware_sdcard_max_freq_khz" "4000" _sd_freq)
    _y("hardware_sdcard_mount_point" "/sdcard" _sd_mount)
    _y("hardware_sdcard_pins_mosi" "-1" _sd_mosi)
    _y("hardware_sdcard_pins_miso" "-1" _sd_miso)
    _y("hardware_sdcard_pins_sclk" "-1" _sd_sclk)

    # Battery
    _b("hardware_battery_enabled" "false" _bat_en)
    _y("hardware_battery_adc_unit" "1" _bat_adc_u)
    _y("hardware_battery_adc_channel" "0" _bat_adc_ch)
    _y("hardware_battery_voltage_max" "4200" _bat_vmax)
    _y("hardware_battery_voltage_min" "3300" _bat_vmin)
    _y("hardware_battery_divider_factor" "200" _bat_div)

    # USB
    _b("hardware_usb_tiny_usb" "false" _usb_tusb)

    # CLI
    _b("cli_enabled" "false" _cli_en)
    _y("cli_prompt" "flxos> " _cli_prompt)
    _y("cli_max_cmdline_length" "256" _cli_maxlen)

    # Capabilities
    _b("capabilities_wifi" "false" _cap_wifi)
    _b("capabilities_bluetooth" "false" _cap_bt)
    _b("capabilities_ble" "false" _cap_ble)
    _b("capabilities_gps" "false" _cap_gps)
    _b("capabilities_lora" "false" _cap_lora)
    _b("capabilities_camera" "false" _cap_cam)
    _b("capabilities_audio" "false" _cap_audio)
    _b("capabilities_keyboard" "false" _cap_kbd)
    _b("capabilities_trackball" "false" _cap_tb)

    # Build the file content
    set(_hpp "// ==========================================================================\n")
    string(APPEND _hpp "// FlxOS Config.hpp — AUTO-GENERATED by profile.cmake\n")
    string(APPEND _hpp "// Profile: ${_id}\n")
    string(APPEND _hpp "// DO NOT EDIT — regenerated on every CMake configure from profile.yaml\n")
    string(APPEND _hpp "// ==========================================================================\n")
    string(APPEND _hpp "#pragma once\n\n")

    # Include SPI header (needed for spi_host_device_t type in structs)
    string(APPEND _hpp "#include <driver/spi_master.h>\n\n")

    # Preprocessor-level feature flags for #if guards (conditional includes)
    # These companion macros exist because #if directives cannot evaluate C++ constexpr.
    # Prefer flx::config::* for all runtime/if-constexpr usage.
    string(APPEND _hpp "// ── Preprocessor feature flags (for #if guards only) ──\n")
    if("${_sd_en}" STREQUAL "true")
        string(APPEND _hpp "#define FLXOS_SD_CARD_ENABLED 1\n")
    else()
        string(APPEND _hpp "#define FLXOS_SD_CARD_ENABLED 0\n")
    endif()
    if("${_bat_en}" STREQUAL "true")
        string(APPEND _hpp "#define FLXOS_BATTERY_ENABLED 1\n")
    else()
        string(APPEND _hpp "#define FLXOS_BATTERY_ENABLED 0\n")
    endif()
    if("${_headless}" STREQUAL "true")
        string(APPEND _hpp "#define FLXOS_HEADLESS 1\n")
    else()
        string(APPEND _hpp "#define FLXOS_HEADLESS 0\n")
    endif()

    if("${_disp_enabled}" STREQUAL "true")
        string(TOUPPER "${_disp_driver}" _drv_upper)
        string(APPEND _hpp "#define FLXOS_DISPLAY_DRIVER_${_drv_upper} 1\n")
        string(TOUPPER "${_disp_bus}" _bus_upper)
        string(APPEND _hpp "#define FLXOS_DISPLAY_BUS_${_bus_upper} 1\n")
    endif()

    if("${_touch_en}" STREQUAL "true")
        string(TOUPPER "${_touch_drv}" _tdrv_upper)
        string(APPEND _hpp "#define FLXOS_TOUCH_DRIVER_${_tdrv_upper} 1\n")
        string(TOUPPER "${_touch_bus}" _tbus_upper)
        string(APPEND _hpp "#define FLXOS_TOUCH_BUS_${_tbus_upper} 1\n")
    endif()

    string(APPEND _hpp "\n")

    string(APPEND _hpp "namespace flx::config {\n\n")

    # ── Struct Definitions ──
    string(APPEND _hpp "// ── Struct Definitions ──\n\n")

    string(APPEND _hpp "struct Profile {\n")
    string(APPEND _hpp "    const char* id;\n")
    string(APPEND _hpp "    const char* vendor;\n")
    string(APPEND _hpp "    const char* boardName;\n")
    string(APPEND _hpp "    const char* target;\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "struct DisplayConfig {\n")
    string(APPEND _hpp "    bool enabled;\n")
    string(APPEND _hpp "    const char* driver;\n")
    string(APPEND _hpp "    const char* bus;\n")
    string(APPEND _hpp "    int width, height, rotation, colorDepth;\n")
    string(APPEND _hpp "    float sizeInches;\n")
    string(APPEND _hpp "    struct { spi_host_device_t host; int mode, freqWrite, freqRead; bool threeWire; int dmaChannel; } spi;\n")
    string(APPEND _hpp "    struct { int port, addr, sda, scl; uint32_t freq; } i2c;\n")
    string(APPEND _hpp "    struct { uint32_t freqWrite, freqRead; int rd, wr, rs; int d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15; } parallel;\n")
    string(APPEND _hpp "    struct { uint32_t freqWrite; int pclk, vsync, hsync, henable; int d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15; int hsyncPulseWidth, hsyncBackPorch, hsyncFrontPorch; int vsyncPulseWidth, vsyncBackPorch, vsyncFrontPorch; bool hsyncPolarity, vsyncPolarity, pclkActiveNeg, deIdleHigh, pclkIdleHigh; } rgb;\n")
    string(APPEND _hpp "    struct { int cs, dc, rst, busy, bckl, mosi, sclk, miso; } pins;\n")
    string(APPEND _hpp "    struct { int offsetX, offsetY, offsetRotation, dummyReadPixel, dummyReadBits;\n")
    string(APPEND _hpp "             bool readable, busShared, invert, rgbOrder, dlen16bit; } panel;\n")
    string(APPEND _hpp "    struct { int freq, pwmChannel; bool invert; } backlight;\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "struct TouchConfig {\n")
    string(APPEND _hpp "    bool enabled;\n")
    string(APPEND _hpp "    const char* driver;\n")
    string(APPEND _hpp "    const char* bus;\n")
    string(APPEND _hpp "    struct { int cs, interrupt, sclk, mosi, miso; } pins;\n")
    string(APPEND _hpp "    struct { int freq; bool busShared, separatePins; spi_host_device_t host; } spi;\n")
    string(APPEND _hpp "    struct { int port, addr, sda, scl; } i2c;\n")
    string(APPEND _hpp "    struct { int xMin, xMax, yMin, yMax, offsetRotation; } calibration;\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "struct SdCardConfig {\n")
    string(APPEND _hpp "    bool enabled;\n")
    string(APPEND _hpp "    int cs;\n")
    string(APPEND _hpp "    spi_host_device_t spiHost;\n")
    string(APPEND _hpp "    int maxFreqKhz;\n")
    string(APPEND _hpp "    const char* mountPoint;\n")
    string(APPEND _hpp "    struct { int mosi, miso, sclk; } pins;\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "struct BatteryConfig {\n")
    string(APPEND _hpp "    bool enabled;\n")
    string(APPEND _hpp "    int adcUnit, adcChannel;\n")
    string(APPEND _hpp "    int voltageMax, voltageMin;\n")
    string(APPEND _hpp "    int dividerFactor;\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "struct UsbConfig {\n")
    string(APPEND _hpp "    bool tinyUsb;\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "struct CliConfig {\n")
    string(APPEND _hpp "    bool enabled;\n")
    string(APPEND _hpp "    const char* prompt;\n")
    string(APPEND _hpp "    int maxCmdlineLength;\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "struct Capabilities {\n")
    string(APPEND _hpp "    bool wifi, bluetooth, ble, gps, lora, camera, audio, keyboard, trackball;\n")
    string(APPEND _hpp "};\n\n")

    # ── Compile-time constants ──
    string(APPEND _hpp "// ── Compile-time constants (auto-generated from profile.yaml) ──\n\n")

    string(APPEND _hpp "inline constexpr Profile profile {\n")
    string(APPEND _hpp "    .id = \"${_id}\",\n")
    string(APPEND _hpp "    .vendor = \"${_vendor}\",\n")
    string(APPEND _hpp "    .boardName = \"${_name_canonical}\",\n")
    string(APPEND _hpp "    .target = \"${_target}\",\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "inline constexpr DisplayConfig display {\n")
    string(APPEND _hpp "    .enabled = ${_disp_enabled},\n")
    string(APPEND _hpp "    .driver = \"${_disp_driver}\",\n")
    string(APPEND _hpp "    .bus = \"${_disp_bus}\",\n")
    string(APPEND _hpp "    .width = ${_disp_w}, .height = ${_disp_h}, .rotation = ${_disp_rot}, .colorDepth = ${_disp_cdepth},\n")
    string(APPEND _hpp "    .sizeInches = ${_disp_size}f,\n")
    string(APPEND _hpp "    .spi = { .host = ${_disp_spi_host}, .mode = ${_disp_spi_mode}, .freqWrite = ${_disp_spi_fw}, .freqRead = ${_disp_spi_fr},\n")
    string(APPEND _hpp "             .threeWire = ${_disp_spi_3w}, .dmaChannel = ${_disp_spi_dma} },\n")
    string(APPEND _hpp "    .i2c = { .port = ${_disp_i2c_port}, .addr = ${_disp_i2c_addr}, .sda = ${_disp_i2c_sda}, .scl = ${_disp_i2c_scl}, .freq = ${_disp_i2c_freq} },\n")
    string(APPEND _hpp "    .parallel = { .freqWrite = ${_disp_par_fw}, .freqRead = ${_disp_par_fr}, .rd = ${_disp_par_rd}, .wr = ${_disp_par_wr}, .rs = ${_disp_par_rs},\n")
    string(APPEND _hpp "                  .d0 = ${_disp_par_d0}, .d1 = ${_disp_par_d1}, .d2 = ${_disp_par_d2}, .d3 = ${_disp_par_d3}, .d4 = ${_disp_par_d4}, .d5 = ${_disp_par_d5}, .d6 = ${_disp_par_d6}, .d7 = ${_disp_par_d7},\n")
    string(APPEND _hpp "                  .d8 = ${_disp_par_d8}, .d9 = ${_disp_par_d9}, .d10 = ${_disp_par_d10}, .d11 = ${_disp_par_d11}, .d12 = ${_disp_par_d12}, .d13 = ${_disp_par_d13}, .d14 = ${_disp_par_d14}, .d15 = ${_disp_par_d15} },\n")
    string(APPEND _hpp "    .rgb = { .freqWrite = ${_disp_rgb_fw}, .pclk = ${_disp_rgb_pclk}, .vsync = ${_disp_rgb_vsync}, .hsync = ${_disp_rgb_hsync}, .henable = ${_disp_rgb_henable},\n")
    string(APPEND _hpp "             .d0 = ${_disp_rgb_d0}, .d1 = ${_disp_rgb_d1}, .d2 = ${_disp_rgb_d2}, .d3 = ${_disp_rgb_d3}, .d4 = ${_disp_rgb_d4}, .d5 = ${_disp_rgb_d5}, .d6 = ${_disp_rgb_d6}, .d7 = ${_disp_rgb_d7},\n")
    string(APPEND _hpp "             .d8 = ${_disp_rgb_d8}, .d9 = ${_disp_rgb_d9}, .d10 = ${_disp_rgb_d10}, .d11 = ${_disp_rgb_d11}, .d12 = ${_disp_rgb_d12}, .d13 = ${_disp_rgb_d13}, .d14 = ${_disp_rgb_d14}, .d15 = ${_disp_rgb_d15},\n")
    string(APPEND _hpp "             .hsyncPulseWidth = ${_disp_rgb_hpw}, .hsyncBackPorch = ${_disp_rgb_hbp}, .hsyncFrontPorch = ${_disp_rgb_hfp},\n")
    string(APPEND _hpp "             .vsyncPulseWidth = ${_disp_rgb_vpw}, .vsyncBackPorch = ${_disp_rgb_vbp}, .vsyncFrontPorch = ${_disp_rgb_vfp},\n")
    string(APPEND _hpp "             .hsyncPolarity = ${_disp_rgb_hp}, .vsyncPolarity = ${_disp_rgb_vp}, .pclkActiveNeg = ${_disp_rgb_pan}, .deIdleHigh = ${_disp_rgb_deih}, .pclkIdleHigh = ${_disp_rgb_pclkih} },\n")
    string(APPEND _hpp "    .pins = { .cs = ${_pin_cs}, .dc = ${_pin_dc}, .rst = ${_pin_rst}, .busy = ${_pin_busy}, .bckl = ${_pin_bckl},\n")
    string(APPEND _hpp "              .mosi = ${_pin_mosi}, .sclk = ${_pin_sclk}, .miso = ${_pin_miso} },\n")
    string(APPEND _hpp "    .panel = { .offsetX = ${_panel_ox}, .offsetY = ${_panel_oy}, .offsetRotation = ${_panel_or},\n")
    string(APPEND _hpp "               .dummyReadPixel = ${_panel_drp}, .dummyReadBits = ${_panel_drb},\n")
    string(APPEND _hpp "               .readable = ${_panel_read}, .busShared = ${_panel_bsh}, .invert = ${_panel_inv},\n")
    string(APPEND _hpp "               .rgbOrder = ${_panel_rgb}, .dlen16bit = ${_panel_d16} },\n")
    string(APPEND _hpp "    .backlight = { .freq = ${_bckl_freq}, .pwmChannel = ${_bckl_ch}, .invert = ${_bckl_inv} },\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "inline constexpr TouchConfig touch {\n")
    string(APPEND _hpp "    .enabled = ${_touch_en},\n")
    string(APPEND _hpp "    .driver = \"${_touch_drv}\",\n")
    string(APPEND _hpp "    .bus = \"${_touch_bus}\",\n")
    string(APPEND _hpp "    .pins = { .cs = ${_tpin_cs}, .interrupt = ${_tpin_int}, .sclk = ${_tpin_sclk}, .mosi = ${_tpin_mosi}, .miso = ${_tpin_miso} },\n")
    string(APPEND _hpp "    .spi = { .freq = ${_tspi_freq}, .busShared = ${_tspi_bsh}, .separatePins = ${_tspi_sep}, .host = ${_tspi_host} },\n")
    string(APPEND _hpp "    .i2c = { .port = ${_ti2c_port}, .addr = ${_ti2c_addr}, .sda = ${_ti2c_sda}, .scl = ${_ti2c_scl} },\n")
    string(APPEND _hpp "    .calibration = { .xMin = ${_tcal_xmin}, .xMax = ${_tcal_xmax}, .yMin = ${_tcal_ymin}, .yMax = ${_tcal_ymax}, .offsetRotation = ${_tcal_or} },\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "inline constexpr SdCardConfig sdcard {\n")
    string(APPEND _hpp "    .enabled = ${_sd_en}, .cs = ${_sd_cs}, .spiHost = ${_sd_spi_host}, .maxFreqKhz = ${_sd_freq},\n")
    string(APPEND _hpp "    .mountPoint = \"${_sd_mount}\", .pins = { .mosi = ${_sd_mosi}, .miso = ${_sd_miso}, .sclk = ${_sd_sclk} },\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "inline constexpr BatteryConfig battery {\n")
    string(APPEND _hpp "    .enabled = ${_bat_en},\n")
    string(APPEND _hpp "    .adcUnit = ${_bat_adc_u}, .adcChannel = ${_bat_adc_ch},\n")
    string(APPEND _hpp "    .voltageMax = ${_bat_vmax}, .voltageMin = ${_bat_vmin},\n")
    string(APPEND _hpp "    .dividerFactor = ${_bat_div},\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "inline constexpr UsbConfig usb { .tinyUsb = ${_usb_tusb} };\n\n")

    string(APPEND _hpp "inline constexpr CliConfig cli {\n")
    string(APPEND _hpp "    .enabled = ${_cli_en},\n")
    string(APPEND _hpp "    .prompt = \"${_cli_prompt}\",\n")
    string(APPEND _hpp "    .maxCmdlineLength = ${_cli_maxlen},\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "inline constexpr bool headless = ${_headless};\n\n")

    string(APPEND _hpp "inline constexpr Capabilities capabilities {\n")
    string(APPEND _hpp "    .wifi = ${_cap_wifi}, .bluetooth = ${_cap_bt}, .ble = ${_cap_ble},\n")
    string(APPEND _hpp "    .gps = ${_cap_gps}, .lora = ${_cap_lora}, .camera = ${_cap_cam},\n")
    string(APPEND _hpp "    .audio = ${_cap_audio}, .keyboard = ${_cap_kbd}, .trackball = ${_cap_tb},\n")
    string(APPEND _hpp "};\n\n")

    string(APPEND _hpp "}  // namespace flx::config\n")

    file(WRITE "${OUTPUT_FILE}" "${_hpp}")
    message(STATUS "FlxOS: Generated ${OUTPUT_FILE}")
endfunction()

# --------------------------------------------------------------------------
# _flx_configure_flash_settings(prefix frag_var stage)
#   Append flash and partition settings into sdkconfig fragment.
#   stage: "size" or "mode_freq"
# --------------------------------------------------------------------------
function(_flx_configure_flash_settings PREFIX FRAG_VAR STAGE)
    set(_frag "${${FRAG_VAR}}")

    if("${STAGE}" STREQUAL "size")
        _flx_yaml_get("${PREFIX}" "flash_size" "4MB" _flash_size)
        string(TOLOWER "${_flash_size}" _fs_lower)

        string(APPEND _frag "\n# Flash Size & Partitions\n")
        string(APPEND _frag "CONFIG_PARTITION_TABLE_CUSTOM=y\n")
        if("${_fs_lower}" STREQUAL "16mb")
            string(APPEND _frag "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions_16mb.csv\"\n")
            string(APPEND _frag "CONFIG_PARTITION_TABLE_FILENAME=\"partitions_16mb.csv\"\n")
            string(APPEND _frag "CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y\n")
            string(APPEND _frag "CONFIG_ESPTOOLPY_FLASHSIZE=\"16MB\"\n")
        elseif("${_fs_lower}" STREQUAL "8mb")
            string(APPEND _frag "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions_8mb.csv\"\n")
            string(APPEND _frag "CONFIG_PARTITION_TABLE_FILENAME=\"partitions_8mb.csv\"\n")
            string(APPEND _frag "CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y\n")
            string(APPEND _frag "CONFIG_ESPTOOLPY_FLASHSIZE=\"8MB\"\n")
        else()
            string(APPEND _frag "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions_4mb.csv\"\n")
            string(APPEND _frag "CONFIG_PARTITION_TABLE_FILENAME=\"partitions_4mb.csv\"\n")
            string(APPEND _frag "CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y\n")
            string(APPEND _frag "CONFIG_ESPTOOLPY_FLASHSIZE=\"4MB\"\n")
        endif()
    elseif("${STAGE}" STREQUAL "mode_freq")
        _flx_yaml_get("${PREFIX}" "flash_mode" "" _flash_mode)
        if(NOT "${_flash_mode}" STREQUAL "" AND NOT "${_flash_mode}" STREQUAL "null")
            string(TOUPPER "${_flash_mode}" _fm_upper)
            string(APPEND _frag "\n# Flash Mode\n")
            string(APPEND _frag "CONFIG_ESPTOOLPY_FLASHMODE_${_fm_upper}=y\n")
        endif()

        _flx_yaml_get("${PREFIX}" "flash_freq" "" _flash_freq)
        if(NOT "${_flash_freq}" STREQUAL "" AND NOT "${_flash_freq}" STREQUAL "null")
            string(APPEND _frag "\n# Flash Frequency\n")
            string(APPEND _frag "CONFIG_ESPTOOLPY_FLASHFREQ_${_flash_freq}=y\n")
        endif()
    else()
        message(FATAL_ERROR "FlxOS: _flx_configure_flash_settings invalid stage '${STAGE}'")
    endif()

    set(${FRAG_VAR} "${_frag}" PARENT_SCOPE)
endfunction()

# --------------------------------------------------------------------------
# _flx_apply_sdkconfig_passthrough(prefix frag_var)
#   Emit raw CONFIG_* key/value pairs from profile.yaml sdkconfig: section.
# --------------------------------------------------------------------------
function(_flx_apply_sdkconfig_passthrough PREFIX FRAG_VAR)
    set(_frag "${${FRAG_VAR}}")
    get_cmake_property(_all_vars VARIABLES)
    set(_has_passthrough FALSE)
    foreach(_var IN LISTS _all_vars)
        if("${_var}" MATCHES "^${PREFIX}_sdkconfig_(.+)")
            if(NOT _has_passthrough)
                string(APPEND _frag "\n# sdkconfig passthrough (from profile.yaml)\n")
                set(_has_passthrough TRUE)
            endif()
            set(_sdk_key "${CMAKE_MATCH_1}")
            string(TOUPPER "${_sdk_key}" _sdk_key_upper)
            string(APPEND _frag "${_sdk_key_upper}=${${_var}}\n")
        endif()
    endforeach()
    set(${FRAG_VAR} "${_frag}" PARENT_SCOPE)
endfunction()

# --------------------------------------------------------------------------
# _flx_generate_sdkconfig_frag()
#   Emit sdkconfig.profile with LVGL, SPIRAM, partition, USB defaults.
# --------------------------------------------------------------------------
function(_flx_generate_sdkconfig_frag PREFIX OUTPUT_FILE)
    macro(_y KEY DEFAULT OUT)
        _flx_yaml_get("${PREFIX}" "${KEY}" "${DEFAULT}" ${OUT})
    endmacro()
    macro(_b KEY DEFAULT OUT)
        _flx_yaml_get("${PREFIX}" "${KEY}" "${DEFAULT}" _raw_${OUT})
        _flx_bool("${_raw_${OUT}}" ${OUT})
    endmacro()

    _b("headless" "false" _headless)
    _y("lvgl_font_size" "14" _font_size)
    _y("lvgl_ui_density" "normal" _ui_density)
    if("${_ui_density}" STREQUAL "" OR "${_ui_density}" STREQUAL "null")
        set(_ui_density "normal")
    endif()
    string(TOLOWER "${_ui_density}" _ui_density_lower)

    set(_frag "# FlxOS sdkconfig.profile — AUTO-GENERATED by profile.cmake\n")
    string(APPEND _frag "# DO NOT EDIT — regenerated on every CMake configure\n\n")

    # LVGL settings (only for non-headless)
    if("${_headless}" STREQUAL "false")
        string(APPEND _frag "# LVGL Configuration\n")
        string(APPEND _frag "CONFIG_LV_CACHE_DEF_SIZE=512000\n")
        string(APPEND _frag "CONFIG_LV_IMAGE_HEADER_CACHE_DEF_CNT=50\n")
        string(APPEND _frag "CONFIG_LV_USE_LOVYAN_GFX=y\n")
        string(APPEND _frag "CONFIG_LV_USE_CLIB_MALLOC=y\n")
        string(APPEND _frag "CONFIG_LV_USE_CLIB_STRING=y\n")
        string(APPEND _frag "CONFIG_LV_USE_CLIB_SPRINTF=y\n")
        string(APPEND _frag "CONFIG_LV_DEF_REFR_PERIOD=10\n")
        string(APPEND _frag "CONFIG_LV_OS_FREERTOS=y\n")
        string(APPEND _frag "CONFIG_LV_OS_IDLE_PERCENT_CUSTOM=y\n")
        string(APPEND _frag "CONFIG_LV_USE_LOG=y\n")
        string(APPEND _frag "CONFIG_LV_LOG_PRINTF=y\n")
        string(APPEND _frag "CONFIG_LV_FS_DEFAULT_DRIVER_LETTER=65\n")
        string(APPEND _frag "CONFIG_LV_USE_FS_STDIO=y\n")
        string(APPEND _frag "CONFIG_LV_FS_STDIO_LETTER=65\n")
        string(APPEND _frag "CONFIG_LV_FS_STDIO_CACHE_SIZE=4096\n")
        string(APPEND _frag "CONFIG_LV_USE_LODEPNG=y\n")
        string(APPEND _frag "CONFIG_LV_USE_TJPGD=y\n")
        string(APPEND _frag "CONFIG_LV_USE_SNAPSHOT=y\n")
        string(APPEND _frag "CONFIG_LV_USE_SYSMON=y\n")
        string(APPEND _frag "CONFIG_LV_USE_PERF_MONITOR=y\n")
        string(APPEND _frag "CONFIG_LV_PERF_MONITOR_ALIGN_TOP_LEFT=y\n")
        string(APPEND _frag "CONFIG_LV_LGFX_USER_INCLUDE=\"../../../../../../HAL/Include/flx/hal/lv_lgfx_user.hpp\"\n")
        string(APPEND _frag "CONFIG_LV_BUILD_EXAMPLES=n\n")
        string(APPEND _frag "CONFIG_LV_BUILD_DEMOS=n\n")

        # 6-tier LVGL font auto-configuration (supports ui_density: normal|compact)
        set(_effective_font_size ${_font_size})
        if("${_ui_density_lower}" STREQUAL "compact")
            math(EXPR _effective_font_size "${_font_size} - 2")
            if(_effective_font_size LESS 10)
                set(_effective_font_size 10)
            endif()
        endif()

        if(_effective_font_size LESS_EQUAL 12)
            set(_f_sm 8)
            set(_f_def 12)
            set(_f_lg 16)
            set(_f_sb 12)
            set(_f_sh 12)
        elseif(_effective_font_size LESS_EQUAL 14)
            set(_f_sm 10)
            set(_f_def 14)
            set(_f_lg 18)
            set(_f_sb 16)
            set(_f_sh 16)
        elseif(_effective_font_size LESS_EQUAL 16)
            set(_f_sm 12)
            set(_f_def 16)
            set(_f_lg 22)
            set(_f_sb 16)
            set(_f_sh 16)
        elseif(_effective_font_size LESS_EQUAL 18)
            set(_f_sm 14)
            set(_f_def 18)
            set(_f_lg 24)
            set(_f_sb 20)
            set(_f_sh 20)
        elseif(_effective_font_size LESS_EQUAL 24)
            set(_f_sm 18)
            set(_f_def 24)
            set(_f_lg 30)
            set(_f_sb 20)
            set(_f_sh 24)
        else()
            set(_f_sm 20)
            set(_f_def 28)
            set(_f_lg 36)
            set(_f_sb 30)
            set(_f_sh 32)
        endif()

        string(APPEND _frag
            "\n# LVGL Font Configuration "
            "(6-tier auto from font_size=${_font_size}, ui_density=${_ui_density_lower}, effective=${_effective_font_size})\n")
        string(APPEND _frag "CONFIG_LV_FONT_MONTSERRAT_${_f_sm}=y\n")
        string(APPEND _frag "CONFIG_LV_FONT_MONTSERRAT_${_f_def}=y\n")
        string(APPEND _frag "CONFIG_LV_FONT_MONTSERRAT_${_f_lg}=y\n")
        string(APPEND _frag "CONFIG_LV_FONT_MONTSERRAT_${_f_sb}=y\n")
        string(APPEND _frag "CONFIG_LV_FONT_MONTSERRAT_${_f_sh}=y\n")
        string(APPEND _frag "CONFIG_LV_FONT_DEFAULT_MONTSERRAT_${_f_def}=y\n")
    endif()

    # Headless mode Kconfig
    if("${_headless}" STREQUAL "true")
        string(APPEND _frag "\n# Headless Mode\n")
        string(APPEND _frag "CONFIG_FLXOS_HEADLESS_MODE=y\n")
    endif()

    # CLI
    _b("cli_enabled" "false" _cli_en)
    if("${_cli_en}" STREQUAL "true")
        string(APPEND _frag "\n# CLI\n")
        string(APPEND _frag "CONFIG_FLXOS_CLI_ENABLED=y\n")
    endif()

    # Profile ID
    _y("id" "" _prof_id)
    string(APPEND _frag "\n# Profile\n")
    string(APPEND _frag "CONFIG_FLXOS_PROFILE=\"${_prof_id}\"\n")

    _flx_configure_flash_settings("${PREFIX}" _frag "size")

    # SPIRAM
    _b("hardware_spiram_enabled" "false" _spiram)
    if("${_spiram}" STREQUAL "true")
        string(APPEND _frag "\n# SPIRAM\n")
        string(APPEND _frag "CONFIG_SPIRAM=y\n")
        string(APPEND _frag "CONFIG_SPIRAM_MODE_OCT=y\n")

        # SPIRAM speed: 80M or 120M (120M requires flash freq to also be 120M)
        _y("hardware_spiram_speed" "80M" _spiram_speed)
        if("${_spiram_speed}" STREQUAL "120M")
            string(APPEND _frag "CONFIG_SPIRAM_SPEED_120M=y\n")
            # Flash and SPIRAM frequencies must match at 120M
            string(APPEND _frag "\n# Flash frequency synced with SPIRAM (must match at 120MHz)\n")
            string(APPEND _frag "CONFIG_ESPTOOLPY_FLASHFREQ_120M=y\n")
        else()
            string(APPEND _frag "CONFIG_SPIRAM_SPEED_80M=y\n")
        endif()

        # Fine-tuning flags
        _b("hardware_spiram_xip_from_psram" "false" _spiram_xip)
        _b("hardware_spiram_ecc_enable" "false" _spiram_ecc)
        if("${_spiram_xip}" STREQUAL "true")
            string(APPEND _frag "CONFIG_SPIRAM_XIP_FROM_PSRAM=y\n")
        endif()
        if("${_spiram_ecc}" STREQUAL "true")
            string(APPEND _frag "CONFIG_SPIRAM_ECC_ENABLE=y\n")
        endif()
    endif()

    # Target-specific CPU/cache config
    _b("target_config_instruction_cache_32kb" "false" _ic32)
    _b("target_config_data_cache_64kb" "false" _dc64)
    _b("target_config_data_cache_line_64b" "false" _dcl64)
    if("${_ic32}" STREQUAL "true" OR "${_dc64}" STREQUAL "true" OR "${_dcl64}" STREQUAL "true")
        _y("target" "esp32" _tgt_for_cache)
        string(TOUPPER "${_tgt_for_cache}" _tgt_upper)
        string(APPEND _frag "\n# Target CPU/Cache Configuration\n")
        if("${_ic32}" STREQUAL "true")
            string(APPEND _frag "CONFIG_${_tgt_upper}_INSTRUCTION_CACHE_32KB=y\n")
        endif()
        if("${_dc64}" STREQUAL "true")
            string(APPEND _frag "CONFIG_${_tgt_upper}_DATA_CACHE_64KB=y\n")
        endif()
        if("${_dcl64}" STREQUAL "true")
            string(APPEND _frag "CONFIG_${_tgt_upper}_DATA_CACHE_LINE_64B=y\n")
        endif()
    endif()

    # Console
    _b("console_usb_serial_jtag" "false" _usb_jtag)
    if("${_usb_jtag}" STREQUAL "true")
        string(APPEND _frag "\n# Console\n")
        string(APPEND _frag "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y\n")
    endif()
    _flx_configure_flash_settings("${PREFIX}" _frag "mode_freq")

    # IDF Target (was previously in sdkconfig.defaults.<target>)
    _y("target" "esp32" _idf_target)
    string(APPEND _frag "\n# IDF Target\n")
    string(APPEND _frag "CONFIG_IDF_TARGET=\"${_idf_target}\"\n")

    _flx_apply_sdkconfig_passthrough("${PREFIX}" _frag)

    file(WRITE "${OUTPUT_FILE}" "${_frag}")
    message(STATUS "FlxOS: Generated ${OUTPUT_FILE}")
endfunction()

# --------------------------------------------------------------------------
# flx_load_profile()
#   Top-level function called from root CMakeLists.txt.
#   Reads profile.yaml → generates Config.hpp + sdkconfig.profile → configures headless mode.
# --------------------------------------------------------------------------
function(flx_load_profile)
    # Determine profile directory
    set(_profile_id "$CACHE{CONFIG_FLXOS_PROFILE}")
    if("${_profile_id}" STREQUAL "")
        # Scan for CONFIG_FLXOS_PROFILE — sdkconfig.defaults is the source of truth,
        # sdkconfig is only a fallback (it may contain stale values from previous builds).
        set(_scan_files
            "${CMAKE_SOURCE_DIR}/sdkconfig.defaults"
        )
        if(DEFINED SDKCONFIG_DEFAULTS)
            foreach(_f IN LISTS SDKCONFIG_DEFAULTS)
                if(IS_ABSOLUTE "${_f}")
                    list(APPEND _scan_files "${_f}")
                else()
                    list(APPEND _scan_files "${CMAKE_SOURCE_DIR}/${_f}")
                endif()
            endforeach()
        endif()
        # Fallback: read from sdkconfig (may be stale after target switches)
        list(APPEND _scan_files "${CMAKE_SOURCE_DIR}/sdkconfig")

        foreach(_sf IN LISTS _scan_files)
            if(EXISTS "${_sf}")
                file(STRINGS "${_sf}" _prof_lines REGEX "^CONFIG_FLXOS_PROFILE=")
                if(_prof_lines)
                    list(GET _prof_lines -1 _prof_line)
                    string(REGEX REPLACE "^CONFIG_FLXOS_PROFILE=\"(.*)\"$" "\\1" _profile_id "${_prof_line}")
                    break()
                endif()
            endif()
        endforeach()
    endif()

    if("${_profile_id}" STREQUAL "")
        message(WARNING "FlxOS: No profile configured. Run idf.py menuconfig.")
        # Set safe defaults so CMake can still configure
        set(HEADLESS_MODE_ENABLED ON PARENT_SCOPE)
        set(FLXOS_HEADLESS_MODE_ENABLED ON CACHE INTERNAL "Resolved FlxOS headless mode")
        return()
    endif()

    set(_profile_dir "${CMAKE_SOURCE_DIR}/Profiles/${_profile_id}")
    set(_profile_yaml "${_profile_dir}/profile.yaml")

    if(NOT EXISTS "${_profile_yaml}")
        message(FATAL_ERROR "FlxOS: Profile '${_profile_id}' not found at ${_profile_yaml}")
    endif()

    message(STATUS "FlxOS: Loading profile '${_profile_id}' from ${_profile_dir}")

    # Parse YAML with inheritance
    _flx_resolve_inheritance("${_profile_dir}" "_FLX")
    _flx_validate_profile("_FLX")

    # ── Target mismatch detection ──
    # Check the CMake cache (not sdkconfig!) for IDF_TARGET — the cache is what
    # actually determines which compiler toolchain is loaded. A stale cache with
    # the wrong target causes silent wrong-compiler builds.
    _flx_yaml_get("_FLX" "target" "esp32" _profile_target)
    set(_cache_file "${CMAKE_BINARY_DIR}/CMakeCache.txt")
    if(EXISTS "${_cache_file}")
        file(STRINGS "${_cache_file}" _cached_target_lines REGEX "^IDF_TARGET:STRING=")
        if(_cached_target_lines)
            list(GET _cached_target_lines -1 _cached_line)
            string(REGEX REPLACE "^IDF_TARGET:STRING=(.*)$" "\\1" _cached_target "${_cached_line}")
            if(NOT "${_cached_target}" STREQUAL "${_profile_target}")
                message(WARNING
                    "\n"
                    "═══════════════════════════════════════════════════════════\n"
                    "  FlxOS: TARGET MISMATCH — AUTO-RECOVERING\n"
                    "═══════════════════════════════════════════════════════════\n"
                    "  Profile '${_profile_id}' requires target: ${_profile_target}\n"
                    "  But CMake cache has IDF_TARGET:            ${_cached_target}\n"
                    "  Wiping build directory...\n"
                    "═══════════════════════════════════════════════════════════\n"
                )

                # Wipe the entire build directory — partial deletion leaves it in
                # a state where idf.py refuses to operate.
                file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}")
                message(STATUS "FlxOS: Deleted build directory")

                # Also remove stale sdkconfig (it references the old target)
                if(EXISTS "${CMAKE_SOURCE_DIR}/sdkconfig")
                    file(REMOVE "${CMAKE_SOURCE_DIR}/sdkconfig")
                    message(STATUS "FlxOS: Deleted stale sdkconfig")
                endif()

                # CMake can't restart itself mid-configure (wrong toolchain already
                # loaded), and calling idf.py from within CMake is recursive. The
                # user must re-run. Use `python flxos.py build` for full automation.
                message(FATAL_ERROR
                    "\n"
                    "═══════════════════════════════════════════════════════════\n"
                    "  FlxOS: BUILD DIRECTORY WIPED — re-run to continue\n"
                    "═══════════════════════════════════════════════════════════\n"
                    "  Run:  idf.py set-target ${_profile_target} && idf.py build\n"
                    "   Or:  python flxos.py build   (fully automatic)\n"
                    "═══════════════════════════════════════════════════════════\n"
                )
            endif()
        endif()
    endif()

    # ── Pre-generation cleanup ──
    # Delete stale generated files for the CURRENT profile and the source root.
    # Only clean the current profile's Config.hpp — deleting other profiles'
    # Config.hpp can break builds when the CMake cache still references them.
    if(EXISTS "${_profile_dir}/Config.hpp")
        file(REMOVE "${_profile_dir}/Config.hpp")
    endif()
    if(EXISTS "${CMAKE_SOURCE_DIR}/sdkconfig.profile")
        file(REMOVE "${CMAKE_SOURCE_DIR}/sdkconfig.profile")
    endif()
    message(STATUS "FlxOS: Cleaned stale generated files")

    # Generate Config.hpp into the profile directory
    set(_config_hpp "${_profile_dir}/Config.hpp")
    _flx_generate_config_hpp("_FLX" "${_config_hpp}")

    # Generate sdkconfig.profile
    set(_sdkconfig_frag "${CMAKE_SOURCE_DIR}/sdkconfig.profile")
    _flx_generate_sdkconfig_frag("_FLX" "${_sdkconfig_frag}")

    # Build SDKCONFIG_DEFAULTS list:
    #   1. sdkconfig.defaults  (base defaults)
    #   2. sdkconfig.profile   (auto-generated from profile.yaml: LVGL, target, fonts, etc.)
    #
    # NOTE: Target-specific settings (SPIRAM, cache, console, IDF_TARGET) are now
    # fully generated into sdkconfig.profile from profile.yaml — no sdkconfig.defaults.<target> needed.
    set(_sdk_defaults "sdkconfig.defaults")
    list(APPEND _sdk_defaults "${_sdkconfig_frag}")
    set(SDKCONFIG_DEFAULTS "${_sdk_defaults}" PARENT_SCOPE)

    # Configure headless mode
    _flx_yaml_get("_FLX" "headless" "false" _raw_headless)
    _flx_bool("${_raw_headless}" _is_headless)

    if("${_is_headless}" STREQUAL "true")
        message(STATUS "FlxOS: Headless mode enabled — excluding lvgl and LovyanGFX")
        set(HEADLESS_MODE_ENABLED ON PARENT_SCOPE)
        set(FLXOS_HEADLESS_MODE_ENABLED ON CACHE INTERNAL "Resolved FlxOS headless mode")
        # EXCLUDE_COMPONENTS must be set in parent scope for project.cmake
        set(EXCLUDE_COMPONENTS ${EXCLUDE_COMPONENTS} lvgl LovyanGFX PARENT_SCOPE)
    else()
        set(HEADLESS_MODE_ENABLED OFF PARENT_SCOPE)
        set(FLXOS_HEADLESS_MODE_ENABLED OFF CACHE INTERNAL "Resolved FlxOS headless mode")
    endif()
endfunction()
