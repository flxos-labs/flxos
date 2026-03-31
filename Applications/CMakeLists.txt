# Applications component — built-in GUI apps (settings, files, calendar, etc.)
set(APP_REQUIRES "")
set(APP_SOURCES "")

if(FLXOS_HEADLESS_MODE_ENABLED STREQUAL "ON")
    message("APPLICATIONS: HEADLESS MODE DETECTED (ON)")
else()
    message("APPLICATIONS: GRAPHICAL MODE DETECTED (OFF)")
    # Auto-discover all application translation units so adding a new app
    # does not require touching CMake.
    if(DEFINED CMAKE_SCRIPT_MODE_FILE)
        file(GLOB_RECURSE APP_SOURCES
            "*.cpp"
        )
    else()
        file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
            "*.cpp"
        )
    endif()
    set(APP_REQUIRES Core Apps UI Connectivity System)
endif()

idf_component_register(
    SRCS ${APP_SOURCES}
    INCLUDE_DIRS "."
    REQUIRES ${APP_REQUIRES}
)
