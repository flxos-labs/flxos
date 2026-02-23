idf_component_register(
    SRCS
        "${CMAKE_CURRENT_LIST_DIR}/Source/hwd/GeneratedInit.cpp"
    INCLUDE_DIRS
        "${CMAKE_CURRENT_LIST_DIR}"
    PRIV_REQUIRES
        esp_driver_gpio
)
