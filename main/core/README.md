# FlxOS Core Architecture

The `core/` directory contains the platform-independent (mostly) logic and systems of the FlxOS operating system.

## Directory Structure

- `apps/`: User-facing applications (Settings, File Manager, etc.)
- `common/`: Shared types, logging, and base utilities
- `connectivity/`: Networking stack (WiFi, Bluetooth, Hotspot)
- `services/`: Background agents (CLI, Filesystem, System Info)
- `system/`: Core system managers (Display, Power, Theme, Time)
- `tasks/`: FreeRTOS task wrappers (GUI task, Resource Monitor)
- `ui/`: User interface framework and Desktop Environment

## Naming Conventions

- **Directories**: `snake_case` (e.g., `system_core/`, `status_bar/`)
- **Files**: `PascalCase` (e.g., `SettingsManager.cpp`)
- **Classes**: `PascalCase` (e.g., `class ThemeEngine`)

## Include Policy

Always include using the full path starting from `core/` when possible, e.g.:
```cpp
#include "core/system/display/DisplayManager.hpp"
```
