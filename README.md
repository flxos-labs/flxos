<p align="center">
  <h1 align="center">FlxOS</h1>
  <p align="center">
    <strong>A Modern Desktop Environment for ESP32 Microcontrollers</strong>
  </p>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License"></a>
  <img src="https://img.shields.io/badge/ESP--IDF-v5.5-green.svg" alt="ESP-IDF">
  <img src="https://img.shields.io/badge/LVGL-v9-orange.svg" alt="LVGL">
  <img src="https://img.shields.io/badge/platform-ESP32%20%7C%20ESP32--S3-lightgrey.svg" alt="Platform">
  <a href="https://github.com/flxos-labs/flxos/actions"><img src="https://github.com/flxos-labs/flxos/actions/workflows/build.yml/badge.svg" alt="Build Status"></a>
</p>

---

## 📖 Overview

**FlxOS** is a high-performance, feature-rich embedded operating environment designed for **ESP32 microcontrollers**. It combines the robustness of the **ESP-IDF** framework with the powerful graphics capabilities of **LVGL 9** and **LovyanGFX** to deliver a responsive, modern desktop-like experience on embedded hardware.

Whether you're building a smart home controller, industrial HMI, or a custom IoT device with a rich user interface, FlxOS provides the foundation you need.

## ✨ Key Features

| Feature | Description |
|---------|-------------|
| 🎨 **Advanced Graphics Engine** | Built on LVGL 9 + LovyanGFX for hardware-accelerated rendering and smooth 60+ FPS animations |
| 📱 **Modular App Architecture** | Scalable application system with built-in app manager for easy extension |
| 🖥️ **Window Manager** | Full-featured window management with multi-app support and dynamic layouts |
| ⚙️ **System Management** | Centralized hardware abstraction layer (HAL) and resource management |
| 🌐 **Connectivity Suite** | Integrated Wi-Fi, network stack, and IoT capabilities |
| 📂 **File System** | FAT filesystem support with SD card and internal flash storage |
| ⚡ **Real-time Performance** | FreeRTOS-based multitasking for efficient parallel processing |
| 🔄 **Display Rotation** | Dynamic screen rotation with automatic layout adjustment |

## 🏗️ Architecture

```
FlxOS/
├── main/
│   ├── core/
│   │   ├── apps/           # Application modules (Settings, Files, etc.)
│   │   ├── common/         # Shared utilities and helpers
│   │   ├── connectivity/   # Wi-Fi, networking, and IoT features
│   │   ├── services/       # System background services (FileSystem, SystemInfo)
│   │   ├── system/         # System managers (Focus, Notification, Time)
│   │   ├── tasks/          # FreeRTOS task definitions
│   │   └── ui/             # Desktop environment and window manager
│   └── hal/                # Hardware Abstraction Layer
├── components/             # External libraries and custom components
├── assets/                 # Static resources (images, fonts)
└── scripts/                # Build and utility scripts
```

## 🚀 Getting Started

### Prerequisites

Ensure you have the following installed:

- **ESP-IDF v5.5+**: [Installation Guide](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/get-started/)
- **CMake**: 3.16 or later
- **Ninja Build System**
- **Python 3.8+**

### Quick Start

```bash
# Clone the repository with submodules
git clone --recursive https://github.com/flxos-labs/flxos.git
cd flxos

# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Configure for your target
idf.py set-target esp32s3  # or esp32

# Build, flash, and monitor
idf.py build flash monitor
```

### Configuration

FlxOS supports multiple ESP32 variants with optimized configurations:

| Target | Flash Size | Partition File |
|--------|------------|----------------|
| ESP32 | 4MB | `partitions_4mb.csv` |
| ESP32 | 8MB | `partitions_8mb.csv` |
| ESP32-S3 | 16MB | `partitions_16mb.csv` |

To customize settings, run:
```bash
idf.py menuconfig
```

## 🛠️ Development

### Building

```bash
# Full build
idf.py build

# Clean build
idf.py fullclean && idf.py build

# Build with verbose output
idf.py build -v
```

### Flashing

```bash
# Flash and monitor
idf.py flash monitor

# Flash only
idf.py flash

# Monitor serial output (exit with Ctrl+])
idf.py monitor
```

### Code Quality

The project includes automated workflows for:
- **Build verification** via GitHub Actions
- **Code linting** with clang-format and clang-tidy

```bash
# Format code
find main -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

## 🔗 Links

- **GitHub Repository**: [https://github.com/flxos-labs/flxos](https://github.com/flxos-labs/flxos)
- **ESP-IDF Documentation**: [https://docs.espressif.com/projects/esp-idf/](https://docs.espressif.com/projects/esp-idf/)
- **LVGL Documentation**: [https://docs.lvgl.io/](https://docs.lvgl.io/)

---

<p align="center">
  Made with ❤️ by <a href="https://github.com/flxos-labs">FlxOS Labs</a>
</p>
