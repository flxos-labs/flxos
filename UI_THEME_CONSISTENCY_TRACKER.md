# UI Theme Consistency Tracker

Reference plan: UI_THEME_CONSISTENCY_ULTIMATE_PLAN.md

## Status Legend
- [ ] Not started
- [~] In progress
- [x] Done

## Phase 1: Foundation
- [ ] Extend ThemeConfig semantic tokens in UI/Include/flx/ui/theming/themes/Themes.hpp
- [ ] Fill semantic tokens for HYPRLAND and MATERIAL in UI/Source/theming/themes/Themes.cpp
- [ ] Add applyThemedText helper in UI/Include/flx/ui/theming/StyleUtils.hpp
- [ ] Add applyThemedBg helper in UI/Include/flx/ui/theming/StyleUtils.hpp
- [ ] Add applyThemedBorder helper in UI/Include/flx/ui/theming/StyleUtils.hpp
- [ ] Add applyThemedStatusColor helper in UI/Include/flx/ui/theming/StyleUtils.hpp
- [ ] Add applyThemedSeparator helper in UI/Include/flx/ui/theming/StyleUtils.hpp
- [ ] Document no-literal-color rule in contributor guidance

## Phase 2: Desktop Core Reactivity
- [ ] Migrate StatusBar overlay and labels to semantic tokens in UI/Source/desktop/modules/status_bar/StatusBar.cpp
- [ ] Add theme-subject observer path for StatusBar visual refresh in UI/Source/desktop/modules/status_bar/StatusBar.cpp
- [ ] Add theme-subject observer path for focused borders in UI/Source/managers/FocusManager.cpp
- [ ] Replace Desktop overlay hardcoded colors in UI/Source/desktop/Desktop.cpp

## Phase 3: Tools
- [ ] Add runtime theme observer updates in Applications/tools/implementation/Calculator.cpp
- [ ] Normalize Calculator touch target constants in Applications/tools/implementation/Calculator.cpp
- [ ] Replace Screenshot hardcoded colors in Applications/tools/implementation/Screenshot.cpp
- [ ] Add Screenshot theme-reactive status color update path in Applications/tools/implementation/Screenshot.cpp
- [ ] Replace Flashlight hardcoded colors in Applications/tools/implementation/Flashlight.cpp

## Phase 4: Other Apps
- [ ] Replace SystemInfo hardcoded gray palette usage in Applications/system_info/SystemInfoApp.cpp
- [ ] Replace ImageViewer hardcoded error color in Applications/image_viewer/ImageViewerApp.cpp

## Phase 5: Guardrails and QA
- [ ] Add script to flag restricted lv_color_hex usage in scripts/
- [ ] Integrate guardrail into scripts/code_quality.sh
- [ ] Validate runtime theme switch on all migrated screens
- [ ] Run formatting and quality checks

## QA Matrix (mark per screen)
- [ ] Desktop
- [ ] Status bar
- [ ] Quick access panel
- [ ] Calculator
- [ ] Screenshot
- [ ] Flashlight
- [ ] System info
- [ ] Image viewer

## Notes
- Keep PRs phase-scoped and small.
- Do not edit third-party submodules under Libraries/lvgl or Libraries/LovyanGFX.
