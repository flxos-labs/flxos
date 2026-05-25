# 🌐 FlxOS Networking Supremacy Plan — Surpassing Tactility

> **Goal**: Transform FlxOS's networking stack from partial coverage (strong WiFi/Hotspot, stub Bluetooth, no ESP-NOW) into a **complete, production-grade, best-in-class** networking layer that surpasses Tactility in every dimension.
>
> **Date Created**: 2026-05-24
> **Target Completion**: v2.0 release
>
> **Baseline Assessment**:
> - **FlxOS Leads**: Hotspot (NAPT routing, bandwidth monitoring, client tracking, auto-shutdown)
> - **Tactility Leads**: Bluetooth (NimBLE, 4 profiles, 886-line HID Host), ESP-NOW (mesh chat), WiFi maturity (auto-connect, dispatcher queue)
> - **Neither Has**: mDNS, MQTT, OTA-over-WiFi, network diagnostics app

## Resolved Design Decisions

| Question | Decision |
|:---|:---|
| **BLE Stack** | NimBLE (smaller footprint, ESP-IDF v5+ default) |
| **Phase Priority** | WiFi hardening & ESP-NOW first, Bluetooth later |
| **Memory Budget** | Bluetooth is compile-time opt-in, gated behind `HAS_BLUETOOTH` in `profile.yaml` |
| **BLE + WiFi Coexistence** | Explicit coexistence management in `ConnectivityManager` |
| **Automated Tests** | Skipped for now — manual verification only |

---

## 📊 Feature Gap Matrix

| Feature | FlxOS Status | Tactility Status | Gap |
|:---|:---:|:---:|:---:|
| WiFi STA connect/disconnect | ✅ Working | ✅ Working | Parity |
| WiFi scan | ✅ Working | ✅ Working | Parity |
| WiFi auto-connect on boot | ✅ Single network | ✅ Multi-network + scan-match | **Tactility leads** |
| WiFi multi-network credentials | ❌ Single SSID only | ✅ Per-SSID NVS storage | **Tactility leads** |
| WiFi radio state machine | ⚠️ Basic enum | ✅ Full state machine + PubSub | **Tactility leads** |
| WiFi AP / Hotspot | ✅ **Advanced** (NAPT, NAT, bandwidth, clients) | ⚠️ Basic (WebServer-only AP) | **FlxOS leads** |
| Bluetooth radio on/off | ⚠️ Stub (no hardware) | ✅ NimBLE stack | **Tactility leads** |
| BT scanning | ❌ Mock | ✅ Full BLE scan + cache | **Tactility leads** |
| BT pairing / bonding | ❌ None | ✅ NimBLE SM + LittleFS persistence | **Tactility leads** |
| BT HID Host (keyboard/mouse input) | ❌ None | ✅ 886-line implementation | **Tactility leads** |
| BT HID Device (act as keyboard) | ❌ None | ✅ Keyboard/Mouse/Gamepad modes | **Tactility leads** |
| BT SPP (Serial Port) | ❌ None | ✅ Nordic UART Service | **Tactility leads** |
| BT MIDI | ❌ None | ✅ BLE MIDI service | **Tactility leads** |
| BT settings persistence | ❌ None | ✅ Properties file + cache | **Tactility leads** |
| ESP-NOW service | ❌ None | ✅ Full service + pub/sub | **Tactility leads** |
| ESP-NOW Chat app | ❌ None | ✅ Working chat app | **Tactility leads** |
| WebServer | ❌ None | ✅ Full dashboard + file browser + REST API | **Tactility leads** |
| mDNS / Service Discovery | ❌ None | ❌ None | Neither |
| MQTT Client | ❌ None | ❌ None | Neither |
| OTA over WiFi | ❌ None | ❌ None | Neither |
| Network Diagnostics App | ❌ None | ❌ None | Neither |

---

## 🏗️ PHASE 1 — WiFi Station Hardening (Weeks 1–3)

> **Objective**: Close the WiFi maturity gap. Tactility's multi-network credential store and sophisticated auto-connect are superior. FlxOS must match and exceed these while preserving its existing Hotspot advantage.

### 1.1 Multi-Network Credential Store

#### [NEW] `Connectivity/Include/flx/connectivity/wifi/WiFiCredentialStore.hpp`
- Store multiple WiFi networks (SSID, password, auto-connect flag, priority, channel)
- JSON-based storage under `/data/connectivity/wifi/networks/`
- API: `save(cred)`, `load(ssid, cred)`, `remove(ssid)`, `loadAll()`, `contains(ssid)`

#### [NEW] `Connectivity/Source/wifi/WiFiCredentialStore.cpp`
- Per-SSID file: `{ssid_hash}.json` with fields: ssid, password, autoConnect, priority, lastConnected timestamp
- Ordered retrieval by priority then recency

### 1.2 Smart Auto-Connect

#### [MODIFY] `Connectivity/Source/wifi/WiFiManager.cpp`
- After each scan, cross-reference results with credential store
- Connect to the highest-priority known network with the strongest signal
- Implement background periodic scan (configurable interval via `m_wifi_scan_interval_subject`)
- On disconnection, automatically retry known networks
- Pause auto-connect when user manually disconnects (resume on manual connect)

### 1.3 WiFi Radio State Machine Enhancement

#### [MODIFY] `Connectivity/Include/flx/connectivity/wifi/WiFiManager.hpp`
- Expand `WiFiStatus` enum to include `RADIO_OFF`, `RADIO_ON_PENDING`, `RADIO_ON`, `RADIO_OFF_PENDING` states
- Add `GotIPCallback` for Hotspot NAT coordination (already exists, verify robustness)

### 1.4 WiFi Settings UI Enhancement

#### [MODIFY] `Applications/settings/wifi/WiFiSettings.hpp`
- Show saved networks list (editable, deletable, reorderable by priority)
- Show "Forget Network" option
- Show auto-connect toggle per network
- Show WiFi info panel: SSID, IP, RSSI, Channel, MAC address

### 1.5 WiFi Event System

#### [NEW] `Connectivity/Include/flx/connectivity/wifi/WiFiEvents.hpp`
- Typed event enum using FlxOS's `EventBus`:
  ```cpp
  enum class WiFiEvent {
      RadioEnabled, RadioDisabled,
      ScanStarted, ScanFinished,
      Connecting, Connected, Disconnected,
      AuthFailed, NotFound
  };
  ```
- Publish via `EventBus::getInstance().publish("connectivity.wifi", data)`

**Phase 1 Score Impact**: WiFi parity achieved. FlxOS retains Hotspot superiority.

---

## 🏗️ PHASE 2 — ESP-NOW Mesh Networking (Weeks 3–5)

> **Objective**: Match Tactility's ESP-NOW service and go beyond with a mesh networking layer.

### 2.1 ESP-NOW Service

#### [NEW] `Connectivity/Include/flx/connectivity/espnow/EspNowManager.hpp`
- Singleton managed by `ConnectivityManager`
- API:
  ```cpp
  esp_err_t enable(const EspNowConfig& config);
  esp_err_t disable();
  bool isEnabled() const;
  bool addPeer(const esp_now_peer_info_t& peer);
  bool send(const uint8_t* mac, const uint8_t* data, size_t len);
  using ReceiveCallback = std::function<void(const esp_now_recv_info_t*, const uint8_t*, int)>;
  uint32_t subscribe(ReceiveCallback cb);
  void unsubscribe(uint32_t id);
  ```

#### [NEW] `Connectivity/Source/espnow/EspNowManager.cpp`
- Initialize ESP-NOW alongside WiFi (STA or AP mode)
- PMK (Primary Master Key) configuration
- Broadcast peer auto-registration
- Thread-safe receive callback dispatch

### 2.2 ESP-NOW WiFi Coexistence

#### [NEW] `Connectivity/Source/espnow/EspNowWiFiCoexistence.cpp`
- When WiFi STA is not active, initialize minimal WiFi in STA or AP mode for ESP-NOW
- When WiFi STA is active, piggyback on existing WiFi init
- Proper cleanup when switching modes

### 2.3 ConnectivityManager ESP-NOW Integration

#### [MODIFY] `Connectivity/Include/flx/connectivity/ConnectivityManager.hpp`
- Add ESP-NOW observables: `m_espnow_enabled_subject`, `m_espnow_peer_count_subject`
- Add delegation methods: `enableEspNow()`, `disableEspNow()`, `espNowSend()`, etc.

#### [MODIFY] `Connectivity/Source/ConnectivityManager.cpp`
- Initialize `EspNowManager` in `onStart()`
- Coordinate WiFi mode transitions with ESP-NOW state

### 2.4 Chat Application (FlxOS-Only Enhancement)

#### [NEW] `Applications/chat/ChatApp.hpp` / `ChatApp.cpp`
- ESP-NOW based local mesh chat
- Features beyond Tactility: message history persistence, device nicknames, message delivery confirmation, typing indicators

**Phase 2 Score Impact**: ESP-NOW parity + FlxOS-exclusive mesh features.

---

## 🏗️ PHASE 3 — Bluetooth Stack (Weeks 5–9)

> **Priority**: CRITICAL — This is the single largest gap. Tactility has ~100K+ lines of BLE code across kernel drivers, platform implementation, and application layer. FlxOS has 41 lines.
>
> **Stack**: NimBLE (confirmed). Compile-time opt-in gated behind `HAS_BLUETOOTH` in `profile.yaml`.

### 3.1 NimBLE Platform Integration

#### [NEW] `Connectivity/Include/flx/connectivity/bluetooth/BleStack.hpp`
- Singleton class that owns the NimBLE host task lifecycle
- `init()` — call `nimble_port_init()`, configure `ble_hs_cfg` (sync callback, reset callback, security manager settings, IO capabilities)
- `deinit()` — graceful teardown
- `setDeviceName(const char* name)` — set GAP device name
- Integrates with `ConnectivityManager` observable subjects

#### [NEW] `Connectivity/Source/bluetooth/BleStack.cpp`
- Implementation: NimBLE host sync callback triggers `ble_svc_gap_init()`, `ble_svc_gatt_init()`
- Security Manager: configure bonding, MITM protection, SC
- Store/restore bonding info via NVS (ESP-IDF's native NimBLE bonding store)
- Entire file gated behind `#if defined(CONFIG_BT_NIMBLE_ENABLED)`

#### [MODIFY] `Connectivity/CMakeLists.txt`
- Add NimBLE component dependency (conditional on `CONFIG_BT_NIMBLE_ENABLED`)
- Add new source files

#### [MODIFY] `Buildscripts/profile.cmake`
- Enable `CONFIG_BT_ENABLED`, `CONFIG_BT_NIMBLE_ENABLED`, `CONFIG_BT_NIMBLE_MAX_CONNECTIONS=3`
- Gate behind `HAS_BLUETOOTH` hardware capability from profile.yaml

### 3.2 BLE Scanning & Peer Discovery

#### [MODIFY] `Connectivity/Include/flx/connectivity/bluetooth/BluetoothManager.hpp`
- Replace stub with full implementation:
  ```cpp
  struct PeerRecord {
      std::array<uint8_t, 6> addr;
      uint8_t addr_type;
      std::string name;
      int8_t rssi;
      bool paired;
      bool connected;
      int profileId = 0;
  };

  esp_err_t startScan(uint32_t duration_ms = 10000);
  esp_err_t stopScan();
  bool isScanning() const;
  std::vector<PeerRecord> getScanResults() const;
  std::vector<PeerRecord> getPairedPeers() const;
  ```
- Add Observable subjects: `m_scanning_subject`, `m_scan_results_subject`

#### [MODIFY] `Connectivity/Source/bluetooth/BluetoothManager.cpp`
- Implement `ble_gap_disc()` with active scanning
- GAP event handler: `BLE_GAP_EVENT_DISC` → cache `PeerRecord` with deduplication
- Thread-safe scan result cache with mutex

### 3.3 BLE Pairing & Bonding

#### [NEW] `Connectivity/Include/flx/connectivity/bluetooth/BondingStore.hpp`
- Persist bonded peer info to FatFS settings (via `SettingsManager`)
- Store: address, address type, name, profile ID, auto-connect flag
- Methods: `save()`, `load()`, `remove()`, `loadAll()`, `addrToHex()`

#### [NEW] `Connectivity/Source/bluetooth/BondingStore.cpp`
- JSON-based per-peer files stored under `/data/bluetooth/peers/`
- Lazy-load with in-memory cache (match Tactility's `BluetoothSettings.cpp` pattern)

### 3.4 BLE HID Host (Keyboard & Mouse Input)

> This is Tactility's crown jewel — 886 lines of GATT service/characteristic discovery, report map parsing, CCCD subscription, keycode-to-LVGL mapping, and mouse cursor indev registration. FlxOS must match this completely.

#### [NEW] `Connectivity/Include/flx/connectivity/bluetooth/HidHostManager.hpp`
- Central-role HID host that connects to external BLE keyboards/mice
- Public API:
  ```cpp
  esp_err_t connect(const std::array<uint8_t, 6>& addr);
  void disconnect();
  bool isConnected() const;
  bool getConnectedPeer(std::array<uint8_t, 6>& addr_out) const;
  ```

#### [NEW] `Connectivity/Source/bluetooth/HidHostManager.cpp`
- Full GATT pipeline:
  1. `ble_gap_connect()` → GAP event callback
  2. `ble_gattc_disc_all_svcs()` → find HID service (UUID 0x1812)
  3. `ble_gattc_disc_all_chrs()` → find Report characteristics (UUID 0x2A4D) + Report Map (UUID 0x2A4B)
  4. `ble_gattc_disc_all_dscs()` → find CCCDs (UUID 0x2902) and Report References (UUID 0x2908)
  5. Read Report References to get report IDs
  6. Read Report Map → parse HID descriptor to classify reports as Keyboard/Mouse/Consumer
  7. Subscribe to CCCDs for notifications
  8. Handle `BLE_GAP_EVENT_NOTIFY_RX` → route to keyboard/mouse handlers
- Keyboard handler: HID keycode → LVGL key mapping, FreeRTOS queue → `lv_indev` keypad
- Mouse handler: relative motion accumulation → `lv_indev` pointer with cursor image
- Security: handle `BLE_GAP_EVENT_ENC_CHANGE`, retry CCCD on auth errors

### 3.5 BLE HID Device (Act as Keyboard/Mouse/Gamepad)

#### [NEW] `Connectivity/Include/flx/connectivity/bluetooth/HidDeviceManager.hpp`
- Peripheral-role: present FlxOS as a BLE HID device
- Modes: `KEYBOARD`, `MOUSE`, `KEYBOARD_MOUSE`, `GAMEPAD`
- API: `start(mode)`, `stop()`, `sendKey()`, `sendKeyboardReport()`, `sendMouseReport()`, `sendGamepadReport()`

#### [NEW] `Connectivity/Source/bluetooth/HidDeviceManager.cpp`
- GATT server with HID service, report characteristics
- Mode-specific report descriptors
- Advertising with appropriate appearance

### 3.6 BLE Serial Port (Nordic UART Service)

#### [NEW] `Connectivity/Include/flx/connectivity/bluetooth/BleSerialManager.hpp`
- SPP-over-BLE using Nordic UART Service (NUS)
- API: `start()`, `stop()`, `write()`, `read()`, `isConnected()`

#### [NEW] `Connectivity/Source/bluetooth/BleSerialManager.cpp`
- GATT server with NUS UUIDs (`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`)
- TX characteristic (notify), RX characteristic (write)
- Ring buffer for received data

### 3.7 BLE MIDI

#### [NEW] `Connectivity/Include/flx/connectivity/bluetooth/BleMidiManager.hpp`
- BLE MIDI service (UUID `03B80E5A-EDE8-4B33-A751-6CE34EC4C700`)
- API: `start()`, `stop()`, `sendMidi()`, `isConnected()`

#### [NEW] `Connectivity/Source/bluetooth/BleMidiManager.cpp`
- GATT server with MIDI I/O characteristic
- Parse incoming MIDI messages, expose via callback

### 3.8 BLE + WiFi Coexistence Manager

#### [MODIFY] `Connectivity/Source/ConnectivityManager.cpp`
- Explicit coexistence management:
  - Pause BT scanning during active WiFi data transfer (large file downloads, OTA)
  - Reduce BT connection interval during heavy WiFi usage
  - Prioritize WiFi for Hotspot NAT routing when active
  - Log coexistence events for diagnostics
- Configuration via `SettingsManager`: `coex_bt_priority` (0=WiFi priority, 1=Balanced, 2=BT priority)

### 3.9 Bluetooth Settings Persistence & UI

#### [MODIFY] `Applications/settings/bluetooth/BluetoothSettings.hpp`
- Replace mock scan results with real scan data from `BluetoothManager`
- Add paired devices list with connect/disconnect/forget actions
- Add profile selection (HID Host, HID Device, SPP, MIDI)
- Add auto-connect toggle per paired device
- Add device name editing
- Add enable-on-boot toggle

#### [NEW] `System/Source/services/BluetoothPersistence.cpp`
- Settings: `bt_enable_on_boot`, `bt_device_name`, `bt_spp_auto_start`, `bt_midi_auto_start`
- Register with `SettingsManager` for automatic persistence

### 3.10 ConnectivityManager Bluetooth Integration

#### [MODIFY] `Connectivity/Include/flx/connectivity/ConnectivityManager.hpp`
- Add BT observables: `m_bt_scanning_subject`, `m_bt_paired_count_subject`
- Add delegation methods: `startBluetoothScan()`, `connectBluetoothHid()`, `startBluetoothSpp()`, etc.
- Add `enableBluetoothOnBoot()` integration in `onStart()`
- All BT code gated behind `#if defined(CONFIG_BT_NIMBLE_ENABLED)`

#### [MODIFY] `Connectivity/Source/ConnectivityManager.cpp`
- Initialize `BleStack` → `BluetoothManager` chain (conditional)
- Auto-enable BT on boot if settings indicate
- Auto-connect to last HID peer

**Phase 3 Score Impact**: Bluetooth gap closed entirely. FlxOS gains feature parity + unified architecture advantage.

---

## 🏗️ PHASE 4 — WebServer & Network Services (Weeks 8–11)

> **Objective**: Build a web management dashboard that matches Tactility's 1800+ line WebServer and adds FlxOS-exclusive features.

### 4.1 HTTP Server Service

#### [NEW] `Services/Include/flx/services/webserver/WebServerService.hpp`
- IService implementation with `onStart()` / `onStop()`
- Configurable port, auth (Basic HTTP Auth), AP mode WiFi

#### [NEW] `Services/Source/webserver/WebServerService.cpp`
- ESP-IDF `httpd` server
- REST API endpoints:
  - `GET /api/system` — chip info, heap, uptime
  - `GET /api/wifi` — WiFi status, scan results
  - `POST /api/wifi/connect` — connect to network
  - `GET /api/hotspot` — hotspot status, clients, bandwidth
  - `GET /api/bluetooth` — BT status, paired devices
  - `GET /api/apps` — installed apps list
  - `GET /fs/list` — file browser
  - `POST /fs/upload` — file upload
  - `POST /admin/reboot` — system reboot

### 4.2 Web Dashboard Assets

#### [NEW] `storage/data/webserver/`
- Single-page application (HTML/CSS/JS)
- Responsive design for desktop and mobile
- Real-time status updates via polling
- File manager with upload/download/delete

### 4.3 AP Mode for WebServer

- Reuse existing `HotspotManager` to start AP mode
- Configure static IP `192.168.4.1` for web dashboard access
- Captive portal redirect (DNS hijack for `flxos.local`)

### 4.4 WebServer Settings UI

#### [NEW] `Applications/settings/webserver/WebServerSettings.hpp`
- Enable/disable toggle
- Port configuration
- Auth enable/disable with username/password
- WiFi mode selection (Station / Access Point)
- AP SSID/password/channel configuration

**Phase 4 Score Impact**: WebServer parity with enhanced REST API.

---

## 🏗️ PHASE 5 — FlxOS-Only Networking Differentiators (Weeks 10–14)

> **Objective**: Features that **no embedded OS has** — this is where FlxOS pulls decisively ahead.

### 5.1 mDNS Service Discovery

#### [NEW] `Connectivity/Include/flx/connectivity/mdns/MdnsManager.hpp`
- Auto-advertise `flxos.local` hostname
- Service discovery for `_http._tcp`, `_mqtt._tcp`
- Find other FlxOS devices on LAN

#### [NEW] `Connectivity/Source/mdns/MdnsManager.cpp`
- ESP-IDF `mdns` component integration
- Start/stop with WiFi connection events

### 5.2 MQTT Client Service

#### [NEW] `Services/Include/flx/services/mqtt/MqttService.hpp`
- Lightweight MQTT 3.1.1 client
- API: `connect()`, `disconnect()`, `publish()`, `subscribe()`
- Configurable broker, port, credentials, TLS
- EventBus integration: publish MQTT messages as system events

#### [NEW] `Services/Source/mqtt/MqttService.cpp`
- ESP-IDF `mqtt_client` component
- Auto-reconnect with exponential backoff
- Persistent session support
- QoS 0/1/2 support

### 5.3 OTA Update over WiFi

#### [NEW] `Services/Include/flx/services/ota/OtaService.hpp`
- Check for updates from configurable URL
- Download and flash firmware image
- Rollback support via ESP-IDF OTA partitions

#### [NEW] `Services/Source/ota/OtaService.cpp`
- HTTPS firmware download with progress reporting
- SHA-256 verification
- `esp_ota_begin()` / `esp_ota_write()` / `esp_ota_end()`
- A/B partition scheme with automatic rollback on boot failure

### 5.4 Network Diagnostics Application

#### [NEW] `Applications/network_diag/NetworkDiagApp.hpp` / `.cpp`
- Unified network status dashboard:
  - WiFi: SSID, IP, gateway, DNS, RSSI signal strength bar, channel, MAC
  - Hotspot: SSID, client count, bandwidth graph, uptime, NAT status
  - Bluetooth: radio state, paired devices, active connections
  - ESP-NOW: enabled status, peer count, message stats
  - Ping utility: ping any IP/hostname and display latency
  - DNS lookup utility

### 5.5 Network Event Notification Integration

#### [MODIFY] `System/Source/managers/NotificationManager.cpp`
- System-level notifications for all connectivity events:
  - WiFi connected/disconnected with SSID
  - Hotspot client connect/disconnect with hostname
  - Bluetooth peer paired/connected/disconnected
  - OTA update available
  - Network error alerts

### 5.6 Captive Portal for First-Time Setup

#### [NEW] `Services/Source/webserver/CaptivePortal.cpp`
- On first boot (no saved WiFi), auto-start AP + captive portal
- DNS hijack redirects all traffic to setup wizard
- Setup wizard: select WiFi network, enter password, set device name
- Redirects to main dashboard after successful setup

**Phase 5 Score Impact**: FlxOS establishes features that Tactility cannot match without significant new development.

---

## 📈 Projected Score Impact

| Feature Area | FlxOS Before | After Ph 1-2 | After Ph 3-4 | After Ph 5 | Tactility |
|:---|:---:|:---:|:---:|:---:|:---:|
| WiFi Station | 7.5 | **9.5** | **9.5** | **9.5** | 9.0 |
| WiFi Hotspot | **9.5** | **9.5** | **9.5** | **9.8** | 4.0 |
| Bluetooth | 1.0 | 1.0 | **9.0** | **9.5** | 9.0 |
| ESP-NOW / Mesh | 0.0 | **8.5** | **8.5** | **9.0** | 8.0 |
| WebServer | 0.0 | 0.0 | **9.0** | **9.5** | 9.0 |
| mDNS / Discovery | 0.0 | 0.0 | 0.0 | **9.0** | 0.0 |
| MQTT | 0.0 | 0.0 | 0.0 | **9.0** | 0.0 |
| OTA | 0.0 | 0.0 | 0.0 | **9.0** | 0.0 |
| Network Diagnostics | 0.0 | 0.0 | 0.0 | **9.0** | 0.0 |
| **Overall Networking** | **3.5** | **5.8** | **8.2** | **9.3** | **7.0** |

---

## 📋 Execution Timeline

| Phase | Description | Weeks | Status | ETA |
|:---:|:---|:---:|:---:|:---:|
| 1 | WiFi Station Hardening | 1–3 | 🔵 Not Started | Week 3 |
| 2 | ESP-NOW Mesh Networking | 3–5 | 🔵 Not Started | Week 5 |
| 3 | Bluetooth Stack (NimBLE) | 5–9 | 🔵 Not Started | Week 9 |
| 4 | WebServer & Network Services | 8–11 | 🔵 Not Started | Week 11 |
| 5 | FlxOS-Only Differentiators | 10–14 | 🔵 Not Started | Week 14 |

### Key Milestones

| Milestone | Target | Status |
|:---|:---|:---:|
| 🏁 Multi-network credential store working | Week 2 | ⬜ |
| 🏁 Smart auto-connect matching Tactility | Week 3 | ⬜ |
| 🏁 ESP-NOW service + chat app | Week 5 | ⬜ |
| 🏁 NimBLE stack initialized + BLE scanning | Week 6 | ⬜ |
| 🏁 BLE HID Host (keyboard/mouse input) | Week 8 | ⬜ |
| 🏁 All 4 BT profiles working | Week 9 | ⬜ |
| 🏁 WebServer + REST API + File browser | Week 10 | ⬜ |
| 🏁 **NETWORKING PARITY** with Tactility | Week 10 | ⬜ |
| 🏁 mDNS + MQTT + OTA | Week 12 | ⬜ |
| 🏁 Network Diagnostics App | Week 13 | ⬜ |
| 🏁 **NETWORKING SUPREMACY** | Week 14 | ⬜ |

---

## ✅ Verification Plan (Manual Only)

- **WiFi**: Connect to 3 different networks, verify credential persistence and auto-reconnect
- **Hotspot + NAT**: Start hotspot, connect phone, verify internet routing still works
- **ESP-NOW**: Two ESP32 devices running FlxOS, send/receive chat messages
- **Bluetooth**: Pair a BLE keyboard (e.g., Logitech K380), type on FlxOS LVGL UI
- **Bluetooth**: Connect FlxOS as HID device to a phone, send keystrokes
- **WebServer**: Open `http://flxos.local` from laptop, browse files, view system info
- **OTA**: Host firmware binary on local HTTP server, trigger OTA update from FlxOS UI
- **mDNS**: Verify `flxos.local` resolves from another device on the same network

---

## 📋 Change Log

| Date | Change | Author |
|:---|:---|:---|
| 2026-05-24 | Initial plan created (Phases 1–5) | FlxOS Team |
| 2026-05-24 | Reordered phases: WiFi+ESP-NOW first, BT later. Confirmed NimBLE. BT compile-time gated. Explicit coex mgmt. Tests skipped. | FlxOS Team |

---

*This is a living document. Update progress checkboxes and phase status as work progresses.*
