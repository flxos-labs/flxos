# FlxOS

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Framework](https://img.shields.io/badge/framework-ESP--IDF-green.svg)
![Graphics](https://img.shields.io/badge/graphics-LVGL-orange.svg)

## Overview

Welcome to the **FlxOS**, a high-performance, feature-rich embedded environment designed for **ESP32 microcontrollers**. This project integrates the robustness of the **ESP-IDF** framework with the visual capabilities of **LVGL** to deliver a responsive and intuitive desktop-like experience on embedded hardware.

## Key Features

- **Advanced GUI Engine**: built on LVGL 9 and LovyanGFX for hardware-accelerated graphics and smooth frame rates.
- **Modular App Architecture**: a scalable application system allowing for easy addition of new capabilities (`core/apps`).
- **System Management**: centralized hardware abstraction and resource management (`core/system`).
- **Robust Connectivity**: integrated Wi-Fi and network stack handling for IoT capabilities (`core/connectivity`).
- **Multitasking Environment**: FreeRTOS-based task scheduling for efficient parallel processing.

## Getting Started

### Prerequisites

Ensure you have the following installed on your development environment:

- **ESP-IDF v5.x**: [Installation Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- **CMake**: 3.16 or later
- **Ninja Build System**

### Installation

1.  **Clone the Repository**
    ```bash
    git clone --recursive https://github.com/YourUsername/FlxOS.git
    cd FlxOS
    ```

2.  **Set up the Environment**
    ```bash
    . $HOME/esp/esp-idf/export.sh
    ```

3.  **Configure the Project**
    ```bash
    idf.py set-target <target> # e.g., esp32, esp32s3
    idf.py menuconfig
    ```

4.  **Build and Flash**
    ```bash
    idf.py build flash monitor
    ```

## Directory Structure

- `main/`: Core source code and entry point.
  - `core/`: System modules, apps, and UI logic.
  - `lv_os_custom.c`: Custom LVGL OS bindings.
- `components/`: External dependencies and custom component libraries.
- `assets/`: Static assets such as images and fonts.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
