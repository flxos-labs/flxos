idf_component_register(
    SRCS "../Firmware/Source/Main.cpp"
    INCLUDE_DIRS "."
    REQUIRES Core Kernel Services Apps Connectivity System UI HAL Applications Firmware
)
