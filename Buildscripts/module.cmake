# FlxOS Module Registration Macro
#
# Wraps idf_component_register() with FlxOS-specific conventions:
# - Automatic Include/ directory as public include
# - Automatic Source/ file globbing
# - Conditional GUI source exclusion for headless mode
# - Component name derived from directory name

function(flx_add_module)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs SOURCES GUI_SOURCES INCLUDE_DIRS REQUIRES PRIV_REQUIRES)
    cmake_parse_arguments(FLX "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Get component name from current directory
    get_filename_component(COMPONENT_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)

    # Default include directory
    if(NOT FLX_INCLUDE_DIRS)
        set(FLX_INCLUDE_DIRS "Include")
    endif()

    # Collect sources
    set(ALL_SOURCES ${FLX_SOURCES})

    # Add GUI sources only if NOT in headless mode
    if(FLX_GUI_SOURCES AND NOT CONFIG_FLXOS_HEADLESS_MODE)
        list(APPEND ALL_SOURCES ${FLX_GUI_SOURCES})
    endif()

    # Register the component
    idf_component_register(
        SRCS ${ALL_SOURCES}
        INCLUDE_DIRS ${FLX_INCLUDE_DIRS}
        REQUIRES ${FLX_REQUIRES}
        PRIV_REQUIRES ${FLX_PRIV_REQUIRES}
    )

    message(STATUS "FlxOS: Registered module '${COMPONENT_NAME}'")
endfunction()
