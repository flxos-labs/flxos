# Core Module

## Purpose

Contains foundational runtime primitives shared across FlxOS modules.

## Structure

- `Core/Include/flx/core/`: public core interfaces.
- `Core/Source/`: implementation of shared utilities.

## Responsibilities

- Logging, observables, event bus, preferences, and utility abstractions.
- Cross-module data and helper types used by upper layers.

## Integration Notes

- Keep dependencies minimal and stable; this layer should remain broadly reusable.
- Avoid introducing feature-specific policy into Core.
