# ESP32 Desktop Environment (OS)

[![Build Status](https://github.com/your-repo/esp32-os/actions/workflows/build.yml/badge.svg)](https://github.com/Itsmeakash248/OS/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x-blue)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
[![LVGL](https://img.shields.io/badge/LVGL-v9.x-orange)](https://lvgl.io)

A sophisticated, high-performance Desktop Environment designed for the **ESP32 series** (S3, S2, C3, C6, and classic ESP32). Powered by **LVGL v9** and **LovyanGFX**, this project provides a complete UI framework featuring window management, a modern glassmorphism aesthetic, and a robust, centralized hardware configuration system.

---

## 📖 Table of Contents
- [🚀 Key Features](#-key-features)
- [🛠 Hardware Support](#-hardware-support)
- [📥 Getting Started](#-getting-started)
- [📂 Project Structure](#-project-structure)
- [📚 Documentation](#-documentation)
- [🗺️ Roadmap](#️-roadmap)
- [🤝 Contributing](#-contributing)
- [📜 License](#-license)

---

## 🚀 Key Features

- **Universal Chip Support**: Native compatibility with **ESP32, S2, S3, C3, C6, and H2**.
- **App Manager**: Robust lifecycle management for multi-tasking internal applications.
- **Window Manager**: Advanced windowing system with focus management, overlapping, and transparency.
- **Modern UI/UX**: Native glassmorphism, high-quality animations, and an integrated theme engine.
- **Universal HAL**: Centralized hardware abstraction via Kconfig—switch hardware without touching code.
- **System Services**: Dynamic Status Bar, App Launcher, Quick Access Panel, and Connectivity Management (WiFi/BT).

## 🛠 Hardware Support

The OS is designed to be highly portable. While it defaults to ESP32-S3 for high-end UI features, it scales down to budget chips.

### Supported Chipsets
| Series | Recommended Use | PSRAM Support |
| :--- | :--- | :--- |
| **ESP32-S3** | Full Glassmorphism / 3D Graphics | High (Octal/Quad) |
| **ESP32-S2** | Standard Window Management | Quad |
| **ESP32-C3/C6** | Minimalist UI / IoT Dashboards | Internal only |
| **ESP32** | Legacy hardware support | Quad |

### Supported Display Controllers
- **ILI Series**: ILI9163, ILI9225, ILI9341, ILI9342, ILI9481, ILI9486, ILI9488.
- **ST Series**: ST7735, ST7789, ST7796.
- **OLED Series**: SSD1306, SSD1327, SSD1331, SSD1351.
- **Specialty**: GC9A01 (Round), SSD1963, NT35510, RM68120, RA8875.

### Supported Touch Controllers
- **SPI**: XPT2046.
- **I2C**: FT5x06/FT6x36, GT911, CSTxxx, CHSC6x, GSLx680, NS2009, STMPE610.

## 📥 Getting Started

### 1. Prerequisites
Ensure you have the **ESP-IDF v5.x** environment installed.

### 2. Clone and Initialize
```bash
git clone --recursive https://github.com/your-repo/esp32-os.git
cd esp32-os
```

### 3. Target Selection
Switch to your specific hardware target before building:
```bash
# Example for ESP32-C3
idf.py set-target esp32c3
```

### 4. Hardware Configuration
All hardware settings (pins, resolution, drivers) are centralized in `menuconfig`.
```bash
idf.py menuconfig
```
## 📂 Project Structure

```text
├── assets/             # Wallpapers, icons, and system data
├── components/         # Drivers and libraries
│   ├── LovyanGFX/      # Display/Touch abstraction layer
│   └── lvgl/           # Graphics library (v9)
├── main/               # Core OS logic
│   ├── core/
│   │   ├── apps/       # Integrated system applications
│   │   ├── system/     # HAL, Task Manager, and Resource Monitoring
│   │   └── ui/         # Window Manager and UI Framework
│   └── main.cpp        # Entry point
├── scripts/            # Documentation and build helpers
└── partitions-16mb.csv # Recommended partition table
```

## 📚 Documentation

Detailed developer documentation is available within the repository:
- **[LVGL v9 Features](LVGL_Features.md)**: Deep dive into the capabilities of the graphics engine.
- **[Full API List](LVGL_full_api_list.md)**: Comprehensive list of available LVGL APIs for app development.
- **[Update API List](scripts/update_api_list.py)**: Script to regenerate API documentation based on current headers.

## 🗺️ Roadmap (Coming Soon)

## 🤝 Contributing
Contributions are welcome! Please submit Pull Requests or open Issues for new feature requests and driver support.

## 📜 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
