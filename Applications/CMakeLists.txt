# Applications component — built-in GUI apps (settings, files, calendar, etc.)
if(NOT FLXOS_HEADLESS_MODE_ENABLED)
    file(GLOB_RECURSE APP_SOURCES
        "settings/*.cpp"
        "files/*.cpp"
        "calendar/*.cpp"
        "text_editor/*.cpp"
        "image_viewer/*.cpp"
        "system_info/*.cpp"
        "tools/*.cpp"
    )

    idf_component_register(
        SRCS ${APP_SOURCES}
        INCLUDE_DIRS "."
        REQUIRES Core Apps UI Connectivity System
    )
else()
    idf_component_register(
        SRCS
        INCLUDE_DIRS "."
    )
endif()
