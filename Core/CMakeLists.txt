idf_component_register(
    SRCS "Source/EventBus.cpp" "Source/Bundle.cpp"
    INCLUDE_DIRS Include
    PRIV_REQUIRES log
)

message(STATUS "FlxOS: Registered Core module (zero-dependency foundation)")
