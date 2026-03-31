# UI Module

## Purpose

Contains shell, desktop, windows, theming, and LVGL-based user interface components.

## Structure

- `UI/Include/flx/ui/`: public UI contracts and helper APIs.
- `UI/Source/`: desktop, components, theming, wallpaper, and shell implementation.

## Responsibilities

- Render and manage graphical interaction flows.
- Provide reusable widgets and desktop modules.
- Integrate with app framework and system observables.

## Integration Notes

- Keep UI task boundaries explicit for concurrency safety.
- Preserve compatibility between graphical mode and headless mode compile gates.
