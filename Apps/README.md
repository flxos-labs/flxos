# Apps Module

## Purpose

Defines app runtime contracts, manifest types, app manager behavior, and intent routing primitives.

## Structure

- `Apps/Include/flx/apps/`: public app framework headers.
- `Apps/Source/`: app manager and runtime implementation.

## Responsibilities

- Manage app lifecycle (start, pause, resume, stop, finish).
- Hold app contracts and metadata structures.
- Provide intent and result flow between apps.

## Integration Notes

- Keep module APIs stable because most application code depends on these interfaces.
- Avoid embedding UI details in this layer; keep it runtime/framework-focused.
