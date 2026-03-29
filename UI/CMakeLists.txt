# UI component — LVGL UI framework, desktop, theming
set(UI_REQUIRES Core Kernel Services Apps Connectivity System)
set(UI_PRIV_REQUIRES "")
set(UI_SOURCES "")

if("$ENV{FLXOS_HEADLESS_MODE_ENABLED}" STREQUAL "ON")
    message("UI: HEADLESS MODE DETECTED (via ENV)")
else()
    message("UI: GRAPHICAL MODE DETECTED (via ENV)")
    file(GLOB_RECURSE UI_SOURCES "Source/*.cpp")
    list(APPEND UI_SOURCES
        "Source/wallpaper/providers/AnimatedGifProvider.cpp"
        "Source/wallpaper/providers/LottieProvider.cpp"
    )
    list(APPEND UI_REQUIRES lvgl)
    list(APPEND UI_PRIV_REQUIRES LovyanGFX HalModule Profiles json)
endif()

idf_component_register(
    SRCS ${UI_SOURCES}
    INCLUDE_DIRS Include
    REQUIRES ${UI_REQUIRES}
    PRIV_REQUIRES ${UI_PRIV_REQUIRES}
)

if(NOT "$ENV{FLXOS_HEADLESS_MODE_ENABLED}" STREQUAL "ON")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_USE_LOVYAN_GFX=1)
endif()
