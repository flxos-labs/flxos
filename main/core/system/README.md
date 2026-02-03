# FlxOS System Managers

The `system/` directory contains specialized managers that handle different aspects of the OS state.

## Specialized Managers

- `display/`: `DisplayManager` handles brightness, rotation, and screen sleep.
- `focus/`: `FocusManager` tracks active UI components and focus state.
- `notification/`: `NotificationManager` handles the system-wide notification queue.
- `settings/`: `SettingsManager` manages persistent JSON configuration.
- `system_core/`: `SystemManager` handles boot flow and hardware initialization.
- `theme/`: `ThemeManager` manages active themes and colors.
- `time/`: `TimeManager` handles RTC and system clock synchronization.

## Architecture

Most managers follow the **Singleton** pattern and use **Observables** to notify the UI or other services of state changes.
