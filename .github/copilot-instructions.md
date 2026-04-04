# Project Guidelines

## Code Style

- C/C++ source of truth is in `Core`, `Kernel`, `Services`, `Apps`, `Applications`, `System`, `Connectivity`, `HalModule`, `UI`, `Firmware`, and `Profiles`.
- Follow `.clang-format` (LLVM-based, tabs). Run `bash scripts/check_format.sh` before finalizing, and `bash scripts/code_format.sh` to auto-fix.
- Naming checks are enforced by `python scripts/check_naming.py`:
  - Types and C++ filenames: PascalCase
  - Functions: camelCase (members may use `m_`)
  - Macros: UPPER_SNAKE_CASE
- Prefer the existing namespace layout (`flx::core`, `flx::apps`, `flx::services`, etc.).
- Treat `Libraries/lvgl` and `Libraries/LovyanGFX` as third-party submodules; avoid edits there unless the task explicitly requires it.

## Architecture

- FlxOS is profile-driven. `Profiles/<id>/profile.yaml` plus `Buildscripts/profile.cmake` controls target, partition defaults, and headless/graphical mode.
- Keep component boundaries clear:
  - `Core`: shared runtime primitives (event bus, logging, preferences, utilities)
  - `Kernel`: task/resource orchestration
  - `Services`: manifest-based lifecycle services
  - `Apps`: app contracts, manifests, intent routing, app manager
  - `Applications`: end-user apps
  - `System`, `HalModule`, `Connectivity`: device/system integration
  - `UI`: shell, windows, LVGL UI components
- `Firmware/Source/Main.cpp` is the runtime entry component. Keep dependencies explicit in each component's `idf_component_register(... REQUIRES ...)`.

## Build And Test

- Use ESP-IDF 5.5.4 and source the environment before build commands: `source $IDF_PATH/export.sh`.
- Prefer FlxOS CLI commands over raw `idf.py` when possible:
  1. `python flxos.py list`
  2. `python flxos.py select <profile-id>`
  3. `python flxos.py build` (or `python flxos.py build --dev` for fast iteration)
  4. `python flxos.py flash --port /dev/ttyUSB0`
- Run `python flxos.py doctor` to verify local setup and toolchain availability.
- For profile work, run `python flxos.py validate <id>` and `python flxos.py hwgen <id>`.
- CI-aligned quality checks:
  - `bash scripts/check_format.sh`
  - `python scripts/analyze_complexity.py`
  - `python scripts/analyze_includes.py`
  - `python scripts/check_naming.py`
  - `python scripts/check_docs.py`
  - `bash scripts/code_quality.sh` (aggregated report)

## Conventions

- Always select the intended profile before building; profile selection synchronizes sdkconfig and target state.
- When switching targets/profiles, expect build and sdkconfig regeneration. Avoid preserving stale generated artifacts.
- Prefer semantic theme tokens via `Themes::GetConfig(...)` or helpers in `UI/Include/flx/ui/theming/StyleUtils.hpp`; avoid introducing new hardcoded `lv_color_hex(...)` values in app/UI code.
- New app pattern:
  - Implement app class and manifest getter in a single `.cpp` under `Applications/<app_name>/`
  - Register the app manifest in `Applications/AppRegistration.cpp`
  - `Applications/CMakeLists.txt` auto-discovers `*.cpp` app sources
- Keep service additions aligned with `ServiceRegistry` lifecycle and dependency declarations.
- Keep path casing stable. CI checks for case-insensitive collisions.

## Documentation

- Main onboarding and command reference: [README.md](../README.md)
- Profile schema and constraints: [Profiles/schema.yaml](../Profiles/schema.yaml)
- CI workflows: [build.yml](workflows/build.yml), [code-quality.yml](workflows/code-quality.yml)
- LVGL references: [LVGL_Features.md](../LVGL_Features.md), [LVGL_full_api_list.md](../LVGL_full_api_list.md)
- Wallpaper engine planning docs: [WALLPAPER_ENGINE_PLAN.md](../WALLPAPER_ENGINE_PLAN.md), [WALLPAPER_ENGINE_TRACKER.md](../WALLPAPER_ENGINE_TRACKER.md)
