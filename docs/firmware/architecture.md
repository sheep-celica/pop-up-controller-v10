# Firmware Architecture

This page is a quick map of where the main firmware pieces live.

## Entry Point

- [src/main.cpp](../../src/main.cpp): startup flow and main loop

## Core Areas

- [include/config.h](../../include/config.h): project-wide configuration and pin assignments
- [src/services/pop_up_control](../../src/services/pop_up_control): higher-level pop-up control logic
- [src/services/inputs](../../src/services/inputs): local and remote input handling
- [src/services/io](../../src/services/io): LEDs, power control, and expander setup
- [src/services/commands](../../src/services/commands): serial command parsing and handlers
- [src/services/logging](../../src/services/logging): statistics, error logging, manufacturing data, and logging helpers
- [src/services/utilities](../../src/services/utilities): helpers such as voltage and temperature reading
- [src/helpers](../../src/helpers): lower-level hardware helpers such as motor and ADC helpers

## Scripts

- [scripts/build_version.py](../../scripts/build_version.py): bumps patch version and updates build timestamp
- [scripts/export_flash_bundle.py](../../scripts/export_flash_bundle.py): builds and exports a flashable firmware bundle

## Suggested Reading Order

If you are new to the codebase, a good reading order is:

1. `src/main.cpp`
2. `include/config.h`
3. `src/services/pop_up_control`
4. `src/services/inputs`
5. `src/services/commands`
