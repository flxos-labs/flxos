# UI Theme Consistency Ultimate Plan

## Goal
Build a fully theme-aware, reactive, and consistent UI system across FlxOS apps and desktop modules.

## Scope
- Theme token model
- Reusable style helpers
- Runtime reactivity to theme switch
- Hardcoded color removal
- Shared sizing/opacity constant adoption
- CI guardrails to prevent regressions

## Current Gaps
1. Theme updates are not fully reactive in several components.
2. Hardcoded colors bypass theme tokens in multiple files.
3. Mixed styling patterns cause inconsistent runtime behavior.
4. Theme tokens are missing semantic state colors and separators.
5. Sizing and opacity constants are not used consistently.

## Phase 1: Foundation (Token Model + Utilities)
### 1.1 Extend Theme Tokens
Update theme config with semantic colors used by app code.
- Add tokens: success, warning, info, muted, separator, overlay_bg, overlay_text, card_border
- Files:
  - UI/Include/flx/ui/theming/themes/Themes.hpp
  - UI/Source/theming/themes/Themes.cpp

### 1.2 Add Reactive Style Helpers
Create reusable helpers to avoid copy-paste one-off styling.
- Add helpers:
  - applyThemedText
  - applyThemedBg
  - applyThemedBorder
  - applyThemedStatusColor
  - applyThemedSeparator
- File:
  - UI/Include/flx/ui/theming/StyleUtils.hpp

### 1.3 Define Usage Rules
- No direct lv_color_hex in app and UI modules except approved low-level locations.
- Use semantic token intent, not palette literals.

## Phase 2: Core Desktop Reactivity
### 2.1 Status Bar
Make overlays and dynamic labels reactive on theme changes, not only data changes.
- File: UI/Source/desktop/modules/status_bar/StatusBar.cpp

### 2.2 Focus Manager
Ensure active/inactive border colors refresh when theme changes.
- File: UI/Source/managers/FocusManager.cpp

### 2.3 Desktop Overlay
Replace hardcoded overlay palette with semantic tokens.
- File: UI/Source/desktop/Desktop.cpp

## Phase 3: Tools App Family
### 3.1 Calculator
- Keep theme-aware styling and add runtime reactivity observers.
- Normalize button touch targets and opacity constants.
- File: Applications/tools/implementation/Calculator.cpp

### 3.2 Screenshot
- Replace hardcoded success/error/muted colors with semantic tokens.
- Ensure status text reacts to theme updates.
- File: Applications/tools/implementation/Screenshot.cpp

### 3.3 Flashlight
- Replace hardcoded dark background with theme surface/muted tokens.
- Keep functional white mode behavior while preserving readable foreground text.
- File: Applications/tools/implementation/Flashlight.cpp

## Phase 4: Additional Apps
### 4.1 System Info
Replace hardcoded neutral grays in cards, separators, captions.
- File: Applications/system_info/SystemInfoApp.cpp

### 4.2 Image Viewer
Replace hardcoded error color with theme error token.
- File: Applications/image_viewer/ImageViewerApp.cpp

## Phase 5: Guardrails and QA
### 5.1 CI Guardrail
Add lint check that flags direct lv_color_hex usage outside approved files.
- Add/extend script under scripts/
- Integrate with code quality pipeline

### 5.2 Validation Matrix
For each migrated screen:
- Open screen on MATERIAL and HYPRLAND themes
- Switch theme while screen is visible
- Verify text/background/border update without reopening the screen
- Verify readability and contrast for normal, warning, and error states

## Prioritized Execution Order
1. Phase 1 (tokens + helpers)
2. Phase 2 (desktop core reactivity)
3. Phase 3 (tools)
4. Phase 4 (remaining apps)
5. Phase 5 (guardrails + verification)

## Acceptance Criteria
- No stale colors after runtime theme switch on migrated screens.
- No direct hardcoded color literals in migrated files.
- All migrated components use semantic theme tokens.
- CI warns on newly introduced hardcoded colors in protected directories.
- Visual behavior is consistent across HYPRLAND and MATERIAL.

## Rollout Strategy
- Keep changes split in small reviewable PRs by phase.
- Ship foundation first, then migrate screens in waves.
- Do not block feature work; allow incremental migration with tracker ownership.
