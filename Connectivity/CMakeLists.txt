set(CONNECTIVITY_SRCS "Source/ConnectivityManager.cpp")
set(CONNECTIVITY_REQUIRES Core Services esp_event mbedtls)
set(CONNECTIVITY_PRIV_REQUIRES nvs_flash esp_netif lwip esp_timer System)

if(FLXOS_HAS_WIFI STREQUAL "true")
    list(APPEND CONNECTIVITY_SRCS
        "Source/wifi/WiFiManager.cpp"
        "Source/wifi/WiFiCredentialStore.cpp"
        "Source/wifi/WiFiProvisioning.cpp"
        "Source/hotspot/HotspotManager.cpp"
    )
    list(APPEND CONNECTIVITY_REQUIRES esp_wifi dhcpserver)
endif()

if(FLXOS_HAS_BLUETOOTH STREQUAL "true")
    list(APPEND CONNECTIVITY_SRCS
        "Source/bluetooth/BluetoothManager.cpp"
    )
endif()

idf_component_register(
    SRCS ${CONNECTIVITY_SRCS}
    INCLUDE_DIRS Include
    REQUIRES ${CONNECTIVITY_REQUIRES}
    PRIV_REQUIRES ${CONNECTIVITY_PRIV_REQUIRES}
)
