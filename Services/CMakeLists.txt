idf_component_register(
    SRCS "Source/ServiceRegistry.cpp"
         "Source/settings/SettingsManager.cpp"
    INCLUDE_DIRS Include
    REQUIRES Core
    PRIV_REQUIRES esp_system esp_timer json
)
