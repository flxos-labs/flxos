# Profiles Component
# This component selects the correct hardware profile directory based on Kconfig

set(PROFILE_DIR "")

if(CONFIG_FLXOS_PROFILE_CYD_2432S028R)
    set(PROFILE_DIR "cyd-2432s028r")
elseif(CONFIG_FLXOS_PROFILE_ESP32S3_ILI9341_XPT)
    set(PROFILE_DIR "esp32s3-ili9341-xpt")
elseif(CONFIG_FLXOS_PROFILE_CUSTOM)
    set(PROFILE_DIR "custom")
else()
    message(WARNING "FlxOS: No device profile selected!")
endif()

if(NOT "${PROFILE_DIR}" STREQUAL "")
    message(STATUS "FlxOS: Using device profile '${PROFILE_DIR}'")
    
    idf_component_register(
        INCLUDE_DIRS "${PROFILE_DIR}"
    )
else()
    idf_component_register()
endif()
