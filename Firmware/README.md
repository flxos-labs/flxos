# Firmware Module

## Purpose

Contains firmware entry wiring and project-level runtime bootstrap configuration.

## Structure

- `Firmware/Source/Main.cpp`: runtime entry point.
- `Firmware/Kconfig.projbuild`: project-level Kconfig additions.

## Responsibilities

- Initialize top-level managers and services at startup.
- Bridge build-time configuration into runtime bootstrap behavior.

## Integration Notes

- Keep startup dependencies explicit and aligned with component `REQUIRES` declarations.
- Preserve deterministic initialization order across profiles.
