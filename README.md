<p align="center">
  <!-- Logo placeholder for future -->
  <h1 align="center">🖥️ FlxOS</h1>
  <p align="center">
    <strong>A Modern Operating System for ESP32 Microcontrollers</strong>
  </p>
  <p align="center">
    <a href="https://github.com/flxos-labs/flxos/actions/workflows/build.yml">
      <img src="https://github.com/flxos-labs/flxos/actions/workflows/build.yml/badge.svg" alt="Build Status">
    </a>
    <a href="https://github.com/flxos-labs/flxos/blob/main/LICENSE">
      <img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License">
    </a>
    <img src="https://img.shields.io/badge/ESP--IDF-v5.5-blue?logo=espressif" alt="ESP-IDF Version">
    <img src="https://img.shields.io/badge/LVGL-v9-purple" alt="LVGL Version">
  </p>
  <p align="center">
    <a href="https://flxos-labs.github.io">Website</a> •
    <a href="#-features">Features</a> •
    <a href="#-quick-start">Quick Start</a> •
    <a href="#-roadmap">Roadmap</a> •
    <a href="#-contributing">Contributing</a>
  </p>
</p>

---

FlxOS is a feature-rich, embedded operating system designed for ESP32 microcontrollers. It provides a complete desktop-like experience with a modern graphical user interface, window management, application lifecycle, connectivity features, and extensive hardware support.

<!-- Screenshot placeholder for future
<p align="center">
  <img src="assets/screenshots/desktop.png" alt="FlxOS Desktop" width="600">
</p>
-->

## ✨ Features

### 🖥️ Core System
- **App Lifecycle Management** — Start, stop, pause, and resume applications seamlessly
- **Window Manager** — Multi-window support with dynamic layouts
- **Task Scheduler** — FreeRTOS-based background task scheduling
- **Resource Monitor** — Real-time CPU and memory monitoring
- **Backend Service Layer** — Decoupled UI from system logic
- **Headless Mode** — Run without display/GUI hardware for IoT scenarios

### 🎨 User Interface
- **Desktop Environment** — Taskbar, app launcher, and status bar
- **Theme Engine** — Dark and light themes with easy customization
- **Virtual Keyboard** — On-screen input for touch displays
- **Notification System** — Toast and popup notifications
- **Display Rotation** — Dynamic orientation support (0°, 90°, 180°, 270°)
- **Custom Wallpapers** — Personalize your desktop

### 🌐 Connectivity
- **WiFi Station Mode** — Connect to wireless networks
- **WiFi Hotspot (SoftAP)** — Create hotspot with NAT and persistent configuration
- **WiFi Scanning** — Network discovery and management
- **Bluetooth Control** — Enable/disable Bluetooth support

### 📱 Built-in Applications
| App | Status | Description |
|-----|--------|-------------|
| ⚙️ Settings | ✅ Complete | System configuration hub |
| 📁 Files | ✅ Complete | File browser with SD card support |
| 📶 WiFi Settings | ✅ Complete | Network management |
| 📺 Display Settings | ✅ Complete | Brightness, rotation controls |
| 📡 Hotspot Settings | ✅ Complete | AP configuration |
| 🔵 Bluetooth Settings | ✅ Complete | BT management |
| ℹ️ System Info | ✅ Complete | Hardware/software information |
| 🧮 Calculator | ⏳ Planned | Basic calculator |
| ⏰ Clock/Alarm | ⏳ Planned | Time, alarms, timer |
| 💻 Terminal | ⏳ Planned | Debug console |

### 💾 Storage & Data
- **Internal Flash (FAT)** — With wear-leveling support
- **SD Card Support** — External storage expansion
- **Settings Persistence** — NVS and JSON-based storage

### 🎨 Hardware Abstraction Layer
- **30+ Display Drivers** — Via LovyanGFX integration
- **Universal Touch Support** — XPT2046, GT911, FT5x06, CST816, and more
- **Flexible Configuration** — Easy hardware customization

---

## 🚀 Quick Start

### Prerequisites

- [ESP-IDF v5.5+](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- Python 3.8+
- Git with submodule support

### Installation

```bash
# Clone the repository with submodules
git clone --recursive https://github.com/flxos-labs/flxos.git
cd flxos

# Set up ESP-IDF environment (if not already done)
. $IDF_PATH/export.sh

# Set target (choose your ESP32 variant)
idf.py set-target esp32s3

# Configure (optional - for custom display/touch setup)
idf.py menuconfig

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

### First Boot

On first boot, FlxOS will:
1. Initialize the display and touch drivers
2. Mount the internal filesystem
3. Launch the desktop environment
4. Show the app launcher with available applications

---

## 🎯 Supported Hardware

### Microcontrollers

FlxOS supports the **entire ESP32 family**:

| MCU | Status | Notes |
|-----|--------|-------|
| ESP32 | ✅ Supported | Original dual-core |
| ESP32-S2 | ✅ Supported | Single-core, USB OTG |
| ESP32-S3 | ✅ Supported | Dual-core, AI acceleration |
| ESP32-C3 | ✅ Supported | RISC-V single-core |
| ESP32-C6 | ✅ Supported | RISC-V, WiFi 6 |
| ESP32-H2 | ✅ Supported | Thread/Zigbee |

### Displays

LovyanGFX provides support for **30+ display controllers**:

- **ILI9341**, **ILI9488**, **ILI9486**, **ILI9481**
- **ST7735**, **ST7789**, **ST7796**
- **GC9A01**, **GC9107**
- **SSD1306**, **SSD1327**, **SSD1351**
- **R61529**, **RM68120**
- And many more...

### Touch Controllers

- **Resistive**: XPT2046, STMPE610
- **Capacitive**: GT911, FT5x06, FT6x36, CST816, CST820

### Partition Sizes

Pre-configured partition tables for various flash sizes:

| Flash Size | Partition File |
|------------|----------------|
| 4 MB | `partitions_4mb.csv` |
| 8 MB | `partitions_8mb.csv` |
| 16 MB | `partitions_16mb.csv` |

---

## 📁 Project Structure

```
flxos/
├── main/
│   ├── core/                 # Platform-independent FlxOS logic
│   │   ├── apps/             # Built-in applications
│   │   ├── common/           # Shared utilities and types
│   │   ├── connectivity/     # WiFi, Bluetooth, Hotspot
│   │   ├── services/         # Background services
│   │   ├── system/           # Core managers (Display, Theme, etc.)
│   │   ├── tasks/            # FreeRTOS task wrappers
│   │   └── ui/               # UI framework and desktop
│   ├── hal/                  # Hardware abstraction layer
│   └── main.cpp              # Entry point
├── components/               # External components
│   ├── lvgl/                 # LVGL graphics library
│   ├── LovyanGFX/            # Display driver library
│   └── dhcpserver/           # DHCP server for hotspot
├── simulator/                # Desktop simulator (SDL)
├── scripts/                  # Build and quality tools
├── assets/                   # Static assets and data
└── .github/workflows/        # CI/CD configuration
```

---

## ⚙️ Configuration

### Menuconfig Options

Access configuration via `idf.py menuconfig`:

```
FlxOS Configuration  --->
    [*] Enable Headless Mode (no GUI)
    Display Settings  --->
        (320) Display Width
        (240) Display Height
        [*] Enable Touch Input
    Storage Settings  --->
        [*] Enable SD Card Support
```

### Headless Mode

For IoT applications that don't require a display:

```bash
idf.py menuconfig
# Navigate to: FlxOS Configuration -> Enable Headless Mode
idf.py build
```

This excludes LVGL and LovyanGFX, significantly reducing binary size.

---

## 🛠️ Development

### Desktop Simulator

Develop and test without hardware using the SDL-based simulator:

```bash
cd simulator
mkdir build && cd build
cmake ..
make
./FlxOS_Simulator
```

### Code Quality

FlxOS uses several tools to maintain code quality:

```bash
# Format code
./scripts/code_format.sh

# Run clang-tidy
./scripts/run_clang_tidy.sh

# Check code quality
./scripts/code_quality.sh
```

### Pre-commit Hooks

Install pre-commit hooks for automatic code quality checks:

```bash
pip install pre-commit
pre-commit install
```

---

## 📍 Roadmap

FlxOS is currently at **v1.0.0 Alpha** with approximately **41% feature completion**.

### Current Milestone (Alpha)
- ✅ Core system framework
- ✅ Window Manager
- ✅ Settings & Files apps
- ✅ WiFi & Hotspot with persistence
- ✅ Theme system
- ✅ Notification System

### Next Milestone (Beta)
- ⏳ Lock screen
- ⏳ OTA updates
- ⏳ Calculator & Terminal apps
- ⏳ Unit tests
- ⏳ API documentation

See the full [**ROADMAP.md**](ROADMAP.md) for detailed progress tracking.

---

## 🤝 Contributing

Contributions are welcome! Here's how you can help:

### Getting Started

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Commit your changes: `git commit -m 'Add amazing feature'`
4. Push to the branch: `git push origin feature/amazing-feature`
5. Open a Pull Request

### Code Style

- **Directories**: `snake_case` (e.g., `system_core/`)
- **Files**: `PascalCase` (e.g., `SettingsManager.cpp`)
- **Classes**: `PascalCase` (e.g., `class ThemeEngine`)

Please run the formatting tools before submitting:

```bash
./scripts/code_format.sh
./scripts/check_format.sh
```

### Areas Needing Help

- 📱 New applications (Calculator, Clock, Terminal)
- 🧪 Unit and integration tests
- 📚 Documentation improvements
- 🌐 Localization/i18n support
- 🔒 Security features

---

## 📄 License

FlxOS is open source software licensed under the [MIT License](LICENSE).

---

## 🙏 Acknowledgments

FlxOS is built upon these excellent open-source projects:

- [**LVGL**](https://lvgl.io/) — Light and Versatile Graphics Library
- [**LovyanGFX**](https://github.com/lovyan03/LovyanGFX) — Display driver library
- [**ESP-IDF**](https://github.com/espressif/esp-idf) — Espressif IoT Development Framework
- [**FreeRTOS**](https://www.freertos.org/) — Real-time operating system kernel

---

<p align="center">
  Made with ❤️ by the FlxOS Team
  <br>
  <a href="https://flxos-labs.github.io">flxos-labs.github.io</a>
</p>
