# Services Module

## Purpose

Hosts manifest-driven runtime services and the service registry lifecycle engine.

## Structure

- `Services/Include/flx/services/`: service contracts and manifests.
- `Services/Source/`: service registry and lifecycle implementation.

## Responsibilities

- Register, sort, initialize, start, and stop services.
- Enforce service dependencies and lifecycle ordering.

## Integration Notes

- New services should declare dependencies explicitly in manifests.
- Keep lifecycle logic deterministic and observable for diagnostics.
