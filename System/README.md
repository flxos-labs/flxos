# System Module

## Purpose

Provides system-level managers and services that coordinate device runtime behavior and platform capabilities.

## Structure

- `System/Include/flx/system/`: public system managers and service APIs.
- `System/Source/`: system manager, managers, and service implementations.

## Responsibilities

- Orchestrate platform-wide settings, diagnostics, storage, and service integration.
- Expose system state and control points to apps and UI.

## Integration Notes

- Keep clear boundaries between orchestration logic and device-specific HAL behavior.
- Prefer explicit contracts when interacting with Connectivity and UI layers.
