# Profiles Component — Auto-Discovery Engine
# Scans for profile.yaml files and registers the active profile's include directory.
# Adding a new profile = adding a folder with profile.yaml. Zero CMake changes.

file(GLOB _profile_yamls LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*/profile.yaml")

set(ACTIVE_PROFILE_DIR "")
foreach(_yaml IN LISTS _profile_yamls)
    get_filename_component(_dir "${_yaml}" DIRECTORY)
    get_filename_component(_name "${_dir}" NAME)
    if("${_name}" STREQUAL "${CONFIG_FLXOS_PROFILE}")
        set(ACTIVE_PROFILE_DIR "${_dir}")
        break()
    endif()
endforeach()

if(ACTIVE_PROFILE_DIR)
    message(STATUS "FlxOS: Active profile '${CONFIG_FLXOS_PROFILE}' → ${ACTIVE_PROFILE_DIR}")
    set(_profile_hwd_generated "${ACTIVE_PROFILE_DIR}/Source/hwd/GeneratedInit.cpp")

    if(EXISTS "${ACTIVE_PROFILE_DIR}/CMakeLists.txt")
        add_subdirectory("${ACTIVE_PROFILE_DIR}")
    elseif(EXISTS "${_profile_hwd_generated}")
        message(STATUS "FlxOS: Optional HWD source enabled → ${_profile_hwd_generated}")
        idf_component_register(
            SRCS "${_profile_hwd_generated}"
            INCLUDE_DIRS "${ACTIVE_PROFILE_DIR}"
        )
    else()
        idf_component_register(INCLUDE_DIRS "${ACTIVE_PROFILE_DIR}")
    endif()
else()
    # Allow menuconfig to work without a profile selected
    if("${CONFIG_FLXOS_PROFILE}" STREQUAL "")
        message(WARNING
            "FlxOS: No profile selected.\n"
            "Run: idf.py menuconfig → FlxOS Configuration → Device Profile ID")
        # Register with a dummy include dir so CMake can still configure
        idf_component_register()
    else()
        message(FATAL_ERROR
            "FlxOS: Profile '${CONFIG_FLXOS_PROFILE}' not found!\n"
            "Available profiles:\n"
            "  ${_profile_yamls}\n"
            "Run: idf.py menuconfig → FlxOS Configuration → Device Profile ID")
    endif()
endif()
