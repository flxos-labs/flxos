# FlxOS Desktop Environment

This module implements the primary user interface and window management for FlxOS.

## Components

- `Desktop`: The main entry point for the UI, managing the wallpaper, screen layout, and module initialization.
- `window_manager/`: Handles window placement, tiling, and application context.
- `modules/`: Standard UI elements:
  - `status_bar/`: System information and clock
  - `dock/`: Quick app switching and launcher access
  - `launcher/`: App drawer
  - `notification_panel/`: System notifications
  - `quick_access_panel/`: Toggles for WiFi, Bluetooth, etc.
  - `swipe_manager/`: Touch gesture handling

## Usage

The `Desktop` is a singleton initialized by the `GuiTask`.

```cpp
Desktop::getInstance().init();
```
