# Profiles Module

## Purpose

Defines hardware and runtime profile metadata that drives target selection, mode gating, and generated build configuration.

## Structure

- `Profiles/schema.yaml`: validation schema for profile definitions.
- `Profiles/_bases/`: reusable profile base fragments.
- `Profiles/<profile-id>/profile.yaml`: concrete profile definitions.

## Responsibilities

- Describe board capabilities and defaults.
- Feed Buildscripts profile processing and generated config output.

## Integration Notes

- Keep profile IDs and path casing stable.
- Validate profile changes before building release artifacts.
