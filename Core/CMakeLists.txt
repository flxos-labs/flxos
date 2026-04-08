idf_component_register(
    SRCS "Source/EventBus.cpp" "Source/Bundle.cpp" "Source/Preferences.cpp" "Source/BootTimeline.cpp"
    INCLUDE_DIRS Include
    REQUIRES fkyaml
    PRIV_REQUIRES log nvs_flash esp_timer
)

message(STATUS "FlxOS: Registered Core module (zero-dependency foundation)")
