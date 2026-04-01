# Connectivity Module

## Purpose

Provides network and wireless functionality, including Wi-Fi station mode, hotspot mode, and protocol-level connectivity services.

## Structure

- `Connectivity/Include/flx/connectivity/`: public connectivity APIs.
- `Connectivity/Source/`: managers and feature implementations.

## Responsibilities

- Manage Wi-Fi scanning, connection, and reconnection behavior.
- Manage hotspot lifecycle and client statistics.
- Expose connectivity state and control interfaces to higher layers.

## Integration Notes

- Keep platform-specific networking details encapsulated in this module.
- Prefer contract/event boundaries for interactions with System-layer settings and orchestration.
