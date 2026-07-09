set(CONNECTIVITY_SRCS "Source/ConnectivityManager.cpp")
set(CONNECTIVITY_REQUIRES Core Services esp_event mbedtls esp_wifi dhcpserver)
set(CONNECTIVITY_PRIV_REQUIRES nvs_flash esp_netif lwip esp_timer System)


message("CONNECTIVITY CMake: FLXOS_HAS_WIFI = '${FLXOS_HAS_WIFI}'")
message("CONNECTIVITY CMake: FLXOS_HAS_BLUETOOTH = '${FLXOS_HAS_BLUETOOTH}'")
message("CONNECTIVITY CMake: BUILD_COMPONENTS = '${BUILD_COMPONENTS}'")

if(FLXOS_HAS_WIFI STREQUAL "true" OR "esp_wifi" IN_LIST BUILD_COMPONENTS)
    message("CONNECTIVITY CMake: Enabling WiFi source files")
    list(APPEND CONNECTIVITY_SRCS
        "Source/wifi/WiFiManager.cpp"
        "Source/wifi/WiFiCredentialStore.cpp"
        "Source/wifi/WiFiProvisioning.cpp"
        "Source/hotspot/HotspotManager.cpp"
    )
endif()

if(FLXOS_HAS_BLUETOOTH STREQUAL "true" OR "bt" IN_LIST BUILD_COMPONENTS)
    message("CONNECTIVITY CMake: Enabling Bluetooth source files")
    list(APPEND CONNECTIVITY_SRCS
        "Source/bluetooth/BluetoothManager.cpp"
    )
endif()

message("CONNECTIVITY CMake: Final CONNECTIVITY_REQUIRES = '${CONNECTIVITY_REQUIRES}'")
message("CONNECTIVITY CMake: Final CONNECTIVITY_PRIV_REQUIRES = '${CONNECTIVITY_PRIV_REQUIRES}'")

idf_component_register(
    SRCS ${CONNECTIVITY_SRCS}
    INCLUDE_DIRS Include
    REQUIRES ${CONNECTIVITY_REQUIRES}
    PRIV_REQUIRES ${CONNECTIVITY_PRIV_REQUIRES}
)
