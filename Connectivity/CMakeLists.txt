idf_component_register(
    SRCS "Source/ConnectivityManager.cpp"
         "Source/wifi/WiFiManager.cpp"
         "Source/bluetooth/BluetoothManager.cpp"
         "Source/hotspot/HotspotManager.cpp"
    INCLUDE_DIRS Include
    REQUIRES Core Services
    PRIV_REQUIRES esp_wifi esp_event esp_netif lwip nvs_flash dhcpserver
)
