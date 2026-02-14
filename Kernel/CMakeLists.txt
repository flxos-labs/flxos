idf_component_register(
    SRCS
        "Source/TaskManager.cpp"
        "Source/ResourceMonitorTask.cpp"
    INCLUDE_DIRS Include
    REQUIRES Core freertos esp_timer
    PRIV_REQUIRES esp_system
)

message(STATUS "FlxOS: Registered Kernel module (Task management and scheduling)")
