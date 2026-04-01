# Kernel Module

## Purpose

Implements task and resource orchestration primitives used by runtime services and apps.

## Structure

- `Kernel/Include/flx/kernel/`: task and resource interfaces.
- `Kernel/Source/`: scheduler helpers, monitors, and orchestration logic.

## Responsibilities

- Coordinate task lifecycle and supervision.
- Expose resource and runtime coordination utilities.

## Integration Notes

- Keep this layer policy-light and focused on orchestration primitives.
- Avoid embedding feature-specific behavior that belongs in Services or System.
