# Applications component — built-in GUI apps (settings, files, calendar, etc.)
set(APP_REQUIRES "")
set(APP_SOURCES "")

if(FLXOS_HEADLESS_MODE_ENABLED STREQUAL "ON")
    message("APPLICATIONS: HEADLESS MODE DETECTED (ON)")
else()
    message("APPLICATIONS: GRAPHICAL MODE DETECTED (OFF)")
    file(GLOB_RECURSE APP_SOURCES
        "settings/*.cpp"
        "files/*.cpp"
        "calendar/*.cpp"
        "text_editor/*.cpp"
        "image_viewer/*.cpp"
        "system_info/*.cpp"
        "tools/*.cpp"
        "wallpaper_engine/*.cpp"
    )
    set(APP_REQUIRES Core Apps UI Connectivity System)
endif()

idf_component_register(
    SRCS ${APP_SOURCES}
    INCLUDE_DIRS "."
    REQUIRES ${APP_REQUIRES}
)
