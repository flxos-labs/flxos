if(DEFINED PROJECT_DIR)
    include(${PROJECT_DIR}/Buildscripts/headless.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/../Buildscripts/headless.cmake)
endif()
flx_resolve_headless(_flx_headless_mode)

if(NOT _flx_headless_mode)
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
