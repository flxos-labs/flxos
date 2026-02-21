if(DEFINED PROJECT_DIR)
    include(${PROJECT_DIR}/Buildscripts/headless.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/../Buildscripts/headless.cmake)
endif()
flx_resolve_headless(_flx_headless_mode)

set(EXTRA_REQS "")

if(NOT _flx_headless_mode)
    list(APPEND EXTRA_REQS UI Applications)
endif()

idf_component_register(
    SRCS "Source/Main.cpp"
    INCLUDE_DIRS "."
    REQUIRES Core Kernel Services Apps Connectivity System HAL Profiles ${EXTRA_REQS}
)

# Inject LovyanGFX dependency into lvgl component
# distinct from standard requirements because we can't modify lvgl's CMakeLists.txt
if(NOT _flx_headless_mode AND CONFIG_LV_USE_LOVYAN_GFX)
    idf_component_get_property(lvgl_lib lvgl COMPONENT_LIB)
    target_link_libraries(${lvgl_lib} PUBLIC idf::LovyanGFX)
    # Profiles provides Config.hpp (or a #error placeholder when no profile is selected)
    target_link_libraries(${lvgl_lib} PUBLIC idf::Profiles)
endif()

if(NOT _flx_headless_mode AND CONFIG_LV_OS_IDLE_PERCENT_CUSTOM)
    idf_component_get_property(lvgl_lib lvgl COMPONENT_LIB)
    target_sources(${lvgl_lib} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../HAL/Source/lv_os_custom.c")
endif()

# Generate Partitions & Upload
fatfs_create_spiflash_image(system ../assets/system FLASH_IN_PROJECT)
fatfs_create_spiflash_image(data ../assets/data FLASH_IN_PROJECT)
