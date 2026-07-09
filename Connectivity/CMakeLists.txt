set(CONNECTIVITY_SRCS "Source/ConnectivityManager.cpp")
set(CONNECTIVITY_REQUIRES Core Services esp_event mbedtls)
set(CONNECTIVITY_PRIV_REQUIRES nvs_flash esp_netif lwip esp_timer System)

if(NOT IDF_TARGET STREQUAL "esp32p4")
    list(APPEND CONNECTIVITY_SRCS
        "Source/wifi/WiFiManager.cpp"
        "Source/wifi/WiFiCredentialStore.cpp"
        "Source/wifi/WiFiProvisioning.cpp"
        "Source/hotspot/HotspotManager.cpp"
        "Source/bluetooth/BluetoothManager.cpp"
    )
    list(APPEND CONNECTIVITY_REQUIRES esp_wifi dhcpserver)
endif()

idf_component_register(
    SRCS ${CONNECTIVITY_SRCS}
    INCLUDE_DIRS Include
    REQUIRES ${CONNECTIVITY_REQUIRES}
    PRIV_REQUIRES ${CONNECTIVITY_PRIV_REQUIRES}
)
