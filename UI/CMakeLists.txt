file(GLOB_RECURSE UI_SOURCES "Source/*.cpp")

idf_component_register(
    SRCS ${UI_SOURCES}
    INCLUDE_DIRS Include
    REQUIRES Core Kernel Services Connectivity System lvgl
    PRIV_REQUIRES LovyanGFX
)

# Add main's HAL display directory for lv_lgfx_user.hpp access
target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_SOURCE_DIR}/main/hal/display")
