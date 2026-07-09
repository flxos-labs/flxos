<p align="center">
  <img src="assets/branding/logo.svg" alt="FlxOS" width="480" />
</p>

<p align="center">
  <strong>Embedded desktop operating system for the ESP32 family.</strong><br/>
  Profile-driven hardware targeting, application runtime, and service orchestration in one platform.
</p>

<p align="center">
  <a href="https://github.com/flxos-labs/flxos/actions/workflows/build.yml"><img src="https://github.com/flxos-labs/flxos/actions/workflows/build.yml/badge.svg" alt="Build Status" /></a>
  <a href="https://github.com/flxos-labs/flxos/actions/workflows/code-quality.yml"><img src="https://github.com/flxos-labs/flxos/actions/workflows/code-quality.yml/badge.svg" alt="Code Quality" /></a>
  <a href="https://github.com/flxos-labs/flxos/releases"><img src="https://img.shields.io/github/v/release/flxos-labs/flxos?include_prereleases&label=release" alt="Release" /></a>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/flxos-labs/flxos" alt="License" /></a>
</p>

---

FlxOS is a production-focused operating system architecture for ESP32-class devices. It combines a desktop-grade UI layer with a structured app and service model, enabling teams to build feature-rich products without ad hoc firmware growth.

### 🖼️ Product Preview

<table>
  <tr>
    <td align="center"><img src="https://raw.githubusercontent.com/flxos-labs/flxos-labs/main/public/images/screenshots/scr_20260312_162354_sleek_app_launcher.png" width="180" alt="App launcher"/><br/><sub>App launcher</sub></td>
    <td align="center"><img src="https://raw.githubusercontent.com/flxos-labs/flxos-labs/main/public/images/screenshots/scr_20260312_162819_notification_panel.png" width="180" alt="Notification panel"/><br/><sub>Notifications</sub></td>
    <td align="center"><img src="https://raw.githubusercontent.com/flxos-labs/flxos-labs/main/public/images/screenshots/scr_20260312_162917_quickaccess_panel.png" width="180" alt="Quick access panel"/><br/><sub>Quick access</sub></td>
    <td align="center"><img src="https://raw.githubusercontent.com/flxos-labs/flxos-labs/main/public/images/screenshots/scr_20260312_163129_open_image_in_image_viewer_from_files_app_side_by_side_with_dynamic_dwindle_layout.png" width="180" alt="Image viewer"/><br/><sub>Multi-window</sub></td>
  </tr>
</table>

---

## ✨ Features

- 🖥️ **Desktop Shell**: Window manager (dwindle layout), app launcher, status bar, notifications, quick access panel, and gesture navigation.
- 📱 **App Framework**: Manifest-driven applications, intent routing, MIME handlers, and URL schemes.
- ⚙️ **Service Lifecycle**: Dependency-aware startup, health checks, watchdog integration, and self-healing.
- 🔌 **Hardware Portability**: Unified YAML-based hardware profile definition per board, supporting ESP32, S2, S3, C3, C6, H2, and P4 chips.
- 📦 **Core Runtime**: EventBus, system preferences, diagnostic tools, and storage management utilities.

---

## ⚡ Quick Start

### Prerequisites
- **ESP-IDF v6.0.2** (installed and sourced: `source $IDF_PATH/export.sh`)
- Python 3.10+ and Git

### 1. Clone the Repository
```bash
git clone --recurse-submodules https://github.com/flxos-labs/flxos.git
cd flxos
```

### 2. Select, Build & Flash
```bash
# List available hardware profiles
python flxos.py list

# Select your board profile (e.g. esp32s3-ili9341-xpt)
python flxos.py select esp32s3-ili9341-xpt

# Build the firmware
python flxos.py build

# Flash to device (adjust port as needed)
python flxos.py flash --port /dev/ttyUSB0
```
*Note: Windows users should replace `/dev/ttyUSB0` with the corresponding COM port (e.g., `COM3`).*

### 3. Monitor Logs
To open the serial monitor and debug output:
```bash
idf.py monitor -p /dev/ttyUSB0
```
*Press `Ctrl+]` to exit.*

---

## 🎛️ CLI Reference

The [`flxos.py`](flxos.py) script is the main entry point for managing the workspace:

| Command | Description |
|---|---|
| `list [--json]` | List all available hardware profiles |
| `select <id>` | Select a profile and configure build targets |
| `build [--all] [--dev]` | Build current/all profiles (`--dev` disables slow optimizations) |
| `flash [--port]` | Flash active build to device |
| `doctor` | Verify environment configuration and dependencies |
| `info <id>` | Show profile details |
| `diff <a> <b> [--json]` | Compare two profiles |
| `hwgen [id] [--all]` | Generate HWD init scaffold from profile.yaml |
| `release <version>` | Package release artifacts |
| `cdn <version>` | Generate ESP Web Tools manifests |
| `new <id>` | Scaffold a new hardware profile |
| `validate [id]` | Validate profile YAML against schema |

---

## 🗂️ Project Structure

FlxOS is structured into modular components. Each module contains its own documentation:

- [Applications/](Applications/README.md) – End-user applications (Calendar, Files, Text Editor, Settings).
- [Apps/](Apps/README.md) – App framework, manifest parser, and intent router.
- [UI/](UI/README.md) – Desktop shell, window managers, themes, and LVGL integrations.
- [Services/](Services/README.md) – Service registry and system daemon lifecycles.
- [System/](System/README.md) – Power, displays, diagnostics, and settings management.
- [Core/](Core/README.md) – EventBus, preferences, logging, and common utilities.
- [HalModule/](HalModule/README.md) – HAL, touch drivers, and peripheral mappings.
- [Connectivity/](Connectivity/README.md) – Wi-Fi, Bluetooth, and networking stack.
- [Profiles/](Profiles/README.md) – YAML hardware target configurations.

---

## 👩‍💻 Developer Guide

### Adding a New App
1. Implement your app code in `Applications/<app_name>/<AppName>App.cpp`.
2. Register the manifest in `Applications/AppRegistration.cpp`.
3. Build the firmware. The build system automatically scans and registers new sources under `Applications/`.
   *See [Applications/README.md](Applications/README.md) for details.*

### Quality Checks
Before submitting a pull request, run the verification scripts:
```bash
bash scripts/code_format.sh   # Format C++ and Python files
bash scripts/code_quality.sh  # Run static analysis and style checks
```

---

## 📄 License

FlxOS is licensed under the **GNU Affero General Public License v3.0** (AGPL-3.0). See [LICENSE](LICENSE) for the full license text.
Third-party notices are located in [NOTICE](NOTICE).
