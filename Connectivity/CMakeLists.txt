idf_component_register(
    SRCS "Source/ConnectivityManager.cpp"
         "Source/wifi/WiFiManager.cpp"
         "Source/bluetooth/BluetoothManager.cpp"
         "Source/hotspot/HotspotManager.cpp"
    INCLUDE_DIRS Include
    REQUIRES Core Services esp_wifi esp_event dhcpserver
    PRIV_REQUIRES nvs_flash esp_netif lwip esp_timer System
)
