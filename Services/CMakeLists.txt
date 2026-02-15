idf_component_register(
    SRCS "Source/ServiceRegistry.cpp"
    INCLUDE_DIRS Include
    REQUIRES Core
    PRIV_REQUIRES esp_system esp_timer json
)
