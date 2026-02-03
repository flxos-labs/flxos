# FlxOS Connectivity Stack

Manages networking and wireless communications.

## Modules

- `wifi/`: `WiFiManager` handles station mode connection and scanning.
- `bluetooth/`: `BluetoothManager` handles NimBLE stack and BLE initialization.
- `hotspot/`: `HotspotManager` manages softAP mode and DHCP server.

## Manager

The `ConnectivityManager` serves as a high-level facade for all connectivity sub-modules, aggregating their status and providing a unified interface for the UI.
