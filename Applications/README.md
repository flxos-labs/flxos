# Applications Module

## Purpose

Contains end-user apps bundled with FlxOS (calendar, settings, tools, wallpaper engine, and others).

## Structure

- `Applications/AppRegistration.cpp`: central app manifest registration.
- `Applications/CMakeLists.txt`: auto-discovers app sources.
- `Applications/<app_name>/`: app implementation files.

## Responsibilities

- Implement UI-facing app behavior.
- Expose app manifests used by the runtime app manager.
- Keep app-specific logic isolated from system services when possible.

## Integration Notes

- New apps should follow the existing single-file app-plus-manifest pattern in `Applications/<app_name>/`.
- Register each new app in `Applications/AppRegistration.cpp`.
