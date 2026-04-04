<p align="center">
  <img src="assets/branding/logo.svg" alt="FlxOS" width="480" />
</p>

<p align="center">
  <strong>Embedded desktop operating system for the ESP32 family.</strong><br/>
  Profile-driven hardware targeting, application runtime, and service orchestration in one platform.
</p>

<p align="center">
  <a href="https://github.com/flxos-labs/flxos/actions/workflows/build.yml"><img src="https://github.com/flxos-labs/flxos/actions/workflows/build.yml/badge.svg" alt="Build" /></a>
  <a href="https://github.com/flxos-labs/flxos/actions/workflows/code-quality.yml"><img src="https://github.com/flxos-labs/flxos/actions/workflows/code-quality.yml/badge.svg" alt="Code Quality" /></a>
  <a href="https://github.com/flxos-labs/flxos/releases"><img src="https://img.shields.io/github/v/release/flxos-labs/flxos?include_prereleases&label=release" alt="Release" /></a>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/flxos-labs/flxos" alt="License" /></a>
</p>

---

## Why FlxOS

FlxOS is a production-focused operating system architecture for ESP32-class devices. It combines a desktop-grade UI layer with a structured app and service model so teams can ship feature-rich products without ad hoc firmware growth.

### Platform Capabilities

| Capability | What you get |
|---|---|
| Desktop shell | Window manager, launcher, dock, status bar, notifications, quick access panel, gesture navigation |
| App framework | Manifest-driven apps, intent routing, MIME handlers, URL schemes, capability declarations |
| Service lifecycle | Dependency-aware startup, health checks, watchdog integration, safe recovery paths |
| Hardware portability | One YAML profile per board, inheritance, validation, and generated HAL initialization |
| Core runtime | EventBus, preferences, diagnostics, storage utilities, headless mode support |

Supported targets include ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6, ESP32-H2, and ESP32-P4.

---

## Product Preview

<table>
  <tr>
    <td align="center"><img src="https://flxos-labs.github.io/assets/images/screenshots/scr_20260312_162354_sleek_app_launcher.png" width="220" alt="App launcher"/><br/><sub>App launcher</sub></td>
    <td align="center"><img src="https://flxos-labs.github.io/assets/images/screenshots/scr_20260312_162819_notification_panel.png" width="220" alt="Notification panel"/><br/><sub>Notification panel</sub></td>
  </tr>
  <tr>
    <td align="center"><img src="https://flxos-labs.github.io/assets/images/screenshots/scr_20260312_162917_quickaccess_panel.png" width="220" alt="Quick access panel"/><br/><sub>Quick access panel</sub></td>
    <td align="center"><img src="https://flxos-labs.github.io/assets/images/screenshots/scr_20260312_163129_open_image_in_image_viewer_from_files_app_side_by_side_with_dynamic_dwindle_layout.png" width="220" alt="Image viewer and files"/><br/><sub>Image Viewer + Files</sub></td>
  </tr>
</table>

---

## Architecture At A Glance

| Layer | Responsibility |
|---|---|
| Applications | End-user apps such as Calendar, Files, Image Viewer, Text Editor, Settings, System Info, and Tools |
| Apps | AppManager, AppManifest, intents, content provider routing, registry |
| UI | Desktop shell, window management, theming, reusable LVGL components |
| Services | Service registry, manifests, lifecycle hooks, health checks |
| System | Display, power, settings, notifications, diagnostics, profile runtime integration |
| Kernel | Task management and resource monitoring |
| Core | EventBus, observables, preferences, logging, utility infrastructure |
| HalModule | Device registry, buses, display, touch, and peripheral abstraction |
| Connectivity | WiFi, Bluetooth, hotspot, and transport management |
| Platform | ESP-IDF, FreeRTOS, LVGL, LovyanGFX |

Key directories: [Applications](Applications), [Apps](Apps), [UI](UI), [Services](Services), [System](System), [Kernel](Kernel), [Core](Core), [HalModule](HalModule), and [Connectivity](Connectivity).

---

## Hardware Profiles

FlxOS board targeting is profile-driven through [Profiles](Profiles). Each board is defined once in YAML, then validated and converted into generated HAL initialization code.

```yaml
# Profiles/esp32s3-ili9341-xpt/profile.yaml
id: esp32s3-ili9341-xpt
target: esp32s3
flash_size: 16MB
inherits: _bases/base-esp32-spi

hardware:
  spiram:
    enabled: true
    speed: 120M
  display:
    driver: ILI9341
    width: 240
    height: 320
  touch:
    driver: XPT2046
```

### Available Profiles

| Profile | SoC | Display | Touch | Flash |
|---|---|---|---|---|
| esp32s3-ili9341-xpt | ESP32-S3 | ILI9341 (SPI) | XPT2046 | 16 MB |
| cyd-2432s028r | ESP32 | ILI9341 | Resistive | 4 MB |
| lilygo-t-hmi | ESP32-S3 | ST7789 | Capacitive | 16 MB |
| generic-esp32 | ESP32 | Headless | N/A | 4 MB |
| generic-esp32s3 | ESP32-S3 | Headless | N/A | 4 MB |

---

## Quick Start

### Prerequisites

| Requirement | Version |
|---|---|
| [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/) | 5.5.4 |
| Python | 3.10+ |
| CMake | 3.16+ |
| clang-format | 18.x (for scripts/check_format.sh and scripts/code_format.sh parity with CI) |

```bash
source $IDF_PATH/export.sh
```

### Build And Flash

```bash
git clone --recurse-submodules https://github.com/flxos-labs/flxos.git
cd flxos

python flxos.py list
python flxos.py select esp32s3-ili9341-xpt

python flxos.py build
python flxos.py flash --port /dev/ttyUSB0
```

### Fast Iteration Mode

```bash
python flxos.py build --dev
```

---

## CLI Reference

Primary entry point: [flxos.py](flxos.py)

| Command | Purpose |
|---|---|
| list [--json] | List available profiles |
| select <id> | Select profile and align target/configuration |
| build [--all] [--dev] | Build active or all profiles |
| flash [--port] | Flash current build |
| validate [id] | Validate profile schema and constraints |
| info <id> | Show profile details |
| diff <a> <b> [--json] | Compare profiles |
| hwgen [id] [--all] | Generate HAL init code |
| new <id> | Scaffold a new profile |
| doctor | Validate local build environment |
| release <version> | Package release artifacts |
| cdn <version> | Generate ESP Web Tools manifests |

---

## Built-In Applications

| Application | Description |
|---|---|
| Calendar | Date and schedule view |
| Files | Browse, copy, delete, and launch files via intents |
| Image Viewer | MIME-aware image rendering |
| Text Editor | File editing with intent-driven open behavior |
| Settings | System configuration and preferences |
| System Info | Runtime diagnostics and hardware stats |
| Tools | Utility suite |
| Wallpaper Engine | Dynamic wallpaper rendering and lifecycle management |

---

## Engineering Workflow

### Add A New App

1. Create one app source file (single .cpp): Applications/<app-name>/<AppName>App.cpp.
2. Define your app class and expose one manifest getter function from that file.
3. Register that manifest in [Applications/AppRegistration.cpp](Applications/AppRegistration.cpp).

Note: [Applications/CMakeLists.txt](Applications/CMakeLists.txt) now auto-discovers all .cpp files under Applications.

### Add A New Service

1. Implement IService lifecycle methods.
2. Define service manifest dependencies and health behavior.
3. Register in the service registry.

### Add A New Board Profile

```bash
python flxos.py new my-board-id
python flxos.py validate my-board-id
python flxos.py hwgen my-board-id
python flxos.py select my-board-id
python flxos.py build
```

### Quality Gates

```bash
bash scripts/code_format.sh
bash scripts/code_quality.sh
python scripts/check_naming.py
python scripts/check_docs.py
python scripts/analyze_complexity.py
python scripts/analyze_includes.py
```

CI workflows are defined in [.github/workflows/build.yml](.github/workflows/build.yml) and [.github/workflows/code-quality.yml](.github/workflows/code-quality.yml).

---

## Additional Documentation

- [LVGL_Features.md](LVGL_Features.md)
- [LVGL_full_api_list.md](LVGL_full_api_list.md)
- [WALLPAPER_ENGINE_PLAN.md](WALLPAPER_ENGINE_PLAN.md)
- [WALLPAPER_ENGINE_TRACKER.md](WALLPAPER_ENGINE_TRACKER.md)
- [NOTICE](NOTICE)

---

## Roadmap

- Embedded ESP32 desktop OS (current)
- Native Linux, macOS, and Windows host support
- Host-based simulator for faster UI and app iteration
- Plugin ecosystem for externally loaded applications
- OTA update workflows

---

## License

FlxOS is licensed under the GNU Affero General Public License v3.0 (AGPL-3.0).
See [LICENSE](LICENSE) for full terms.
