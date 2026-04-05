# UI Theme Consistency Tracker

Reference plan: UI_THEME_CONSISTENCY_ULTIMATE_PLAN.md

## Status Legend
- [ ] Not started
- [~] In progress
- [x] Done

## Phase 1: Foundation
- [x] Extend ThemeConfig semantic tokens in UI/Include/flx/ui/theming/themes/Themes.hpp
- [x] Fill semantic tokens for HYPRLAND and MATERIAL in UI/Source/theming/themes/Themes.cpp
- [x] Add applyThemedText helper in UI/Include/flx/ui/theming/StyleUtils.hpp
- [x] Add applyThemedBg helper in UI/Include/flx/ui/theming/StyleUtils.hpp
- [x] Add applyThemedBorder helper in UI/Include/flx/ui/theming/StyleUtils.hpp
- [x] Add applyThemedStatusColor helper in UI/Include/flx/ui/theming/StyleUtils.hpp
- [x] Add applyThemedSeparator helper in UI/Include/flx/ui/theming/StyleUtils.hpp
- [x] Document no-literal-color rule in contributor guidance

## Phase 2: Desktop Core Reactivity
- [x] Migrate StatusBar overlay and labels to semantic tokens in UI/Source/desktop/modules/status_bar/StatusBar.cpp
- [x] Add theme-subject observer path for StatusBar visual refresh in UI/Source/desktop/modules/status_bar/StatusBar.cpp
- [x] Add theme-subject observer path for focused borders in UI/Source/managers/FocusManager.cpp
- [x] Replace Desktop overlay hardcoded colors in UI/Source/desktop/Desktop.cpp

## Phase 3: Tools
- [x] Add runtime theme observer updates in Applications/tools/implementation/Calculator.cpp
- [x] Normalize Calculator touch target constants in Applications/tools/implementation/Calculator.cpp
- [x] Replace Screenshot hardcoded colors in Applications/tools/implementation/Screenshot.cpp
- [x] Add Screenshot theme-reactive status color update path in Applications/tools/implementation/Screenshot.cpp
- [x] Replace Flashlight hardcoded colors in Applications/tools/implementation/Flashlight.cpp

## Phase 4: Other Apps
- [x] Replace SystemInfo hardcoded gray palette usage in Applications/system_info/SystemInfoApp.cpp
- [x] Replace ImageViewer hardcoded error color in Applications/image_viewer/ImageViewerApp.cpp

## Phase 5: Guardrails and QA
- [x] Add script to flag restricted lv_color_hex usage in scripts/
- [x] Integrate guardrail into scripts/code_quality.sh
- [x] Validate runtime theme switch on all migrated screens
- [x] Run formatting and quality checks

## QA Matrix (mark per screen)
- [x] Desktop
- [x] Status bar
- [x] Quick access panel
- [x] Calculator
- [x] Screenshot
- [x] Flashlight
- [x] System info
- [x] Image viewer

## Notes
- Keep PRs phase-scoped and small.
- Do not edit third-party submodules under Libraries/lvgl or Libraries/LovyanGFX.
- Formatting check run on 2026-04-04 failed due pre-existing style drift including Profiles/esp32s3-ili9341-xpt/Config.hpp.
- Code quality run on 2026-04-04 executed successfully but reported failing checks: Format, Complexity, Include Analysis, Naming, Documentation, Wallpaper Preset Assets.
- Literal color guardrail run on 2026-04-04 passed.
