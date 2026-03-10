idf_component_register(
    SRCS "Source/EventBus.cpp" "Source/Bundle.cpp" "Source/Preferences.cpp"
    INCLUDE_DIRS Include
    PRIV_REQUIRES log nvs_flash
)

message(STATUS "FlxOS: Registered Core module (zero-dependency foundation)")
