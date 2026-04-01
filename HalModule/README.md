# HalModule Module

## Purpose

Provides hardware abstraction implementations for displays, input, sensors, storage, and low-level peripherals.

## Structure

- `HalModule/Include/flx/hal/`: public HAL interfaces and adapters.
- `HalModule/Source/`: concrete hardware implementations.

## Responsibilities

- Encapsulate ESP32 and board-specific hardware behavior.
- Provide stable device/service-facing APIs to upper layers.

## Integration Notes

- Keep board and driver details localized to this module.
- Avoid leaking vendor-specific assumptions into higher-level module contracts.
