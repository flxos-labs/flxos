idf_component_register(
    SRCS
        "${CMAKE_CURRENT_LIST_DIR}/Source/hwd/GeneratedInit.cpp"
    INCLUDE_DIRS
        "${CMAKE_CURRENT_LIST_DIR}"
    PRIV_REQUIRES Core
)

# Bypass static dependency analyzer bug for included CMakeLists.txt
idf_component_optional_requires(PRIVATE esp_driver_gpio driver)
