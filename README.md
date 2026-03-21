# Pop-up Controller V10

Firmware and reference source for the Pop-up Controller V10 board.

This controller is designed first and foremost for 5th generation Toyota Celica T18 models as a direct replacement for the factory Light Retractor Relay. The hardware and firmware should also be usable on other pop-up headlight cars with a custom wiring adapter, but that is not the main target yet.

![Pop-up Controller V10 PCB](docs/assets/images/controller_pcb.png)

## What This Project Is

This repository exists mainly so owners can see how the controller works, inspect the firmware, and make small changes if they want to.

The firmware runs on an ESP32-based controller board and adds features beyond the factory relay, including:

- pop-up headlight control from the factory light switch inputs
- wink control for right, left, or both pop-ups
- sleepy-eye mode with adjustable position
- optional external remote inputs
- stored calibration, persistent settings and statistics in NVS
- serial commands for communication with the [Pop-up Controller V10 Application](https://github.com/sheep-celica/Pop-up-controller-V10-Application)
- bench mode for safer powered testing below car-battery voltage

## Compatibility

- Primary target: Toyota Celica T18 / 5th generation Celica
- Intended use: direct replacement for the factory Light Retractor Relay
- Possible future compatibility: other pop-up headlight cars with a custom adapter harness

## Current Status

- Schematics are not published yet.
- The source code is available for inspection and minor customization.
- More detailed documentation now lives in [docs/](docs/README.md) and will continue to grow over time.

Useful documentation pages:

- [Documentation Index](docs/README.md)
- [Installation](docs/installation.md)
- [Application](docs/application.md)
- [Hardware Overview](docs/hardware/overview.md)
- [Build and Flash](docs/firmware/build-and-flash.md)
- [Serial Commands](docs/firmware/serial-commands.md)

## Hardware Overview

At a high level, the controller is built around an ESP32 DevKit V1 and a few supporting parts that handle sensing, I/O expansion, and diagnostics.

Some notable hardware in the current design:

- ESP32 DevKit V1 as the main controller
- ADS7138 internal I/O expander and ADC for on-board analog and digital signals
- PCF8574 external expander support for optional remote inputs
- LM75 temperature sensor for board-temperature monitoring
- IX4427N from IXYS as the MOSFET driver for the braking NMOS stage
- Infineon IPD70N10S3-12 as the pop-up braking NMOS
- BTS6163D as the power switch for the pop-up motors
- Vishay SM8S18AHE3_A/I as the main TVS protection diode
- Panasonic EEUFR1V471B as the power-input bulk capacitor
- additional support circuitry for illumination control, buttons, switch inputs, and vehicle-facing I/O

More board-level details and component notes can move into `docs/` later as that documentation grows.

## Main Behavior

The firmware monitors the headlight switch inputs and moves both pop-ups up or down accordingly. It also supports dedicated buttons and optional remote inputs for wink and sleepy-eye functions.

Some notable behavior:

- when supply voltage is below 7 V, the controller enters bench mode and disables pop-up movement
- settings such as sleepy-eye safety behavior, remote-input behavior, idle power-off timeout, and calibration values are stored persistently
- serial commands are only processed while the pop-ups are idle

## Build From Source

This is a PlatformIO project targeting `esp32doit-devkit-v1`.

Dependencies are managed through [platformio.ini](platformio.ini) and currently include:

- LM75 temperature sensor library
- PCF8574 library

Typical workflow:

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

## Flashing A Prebuilt Firmware Bundle

If you do not want to build from source, download a prepared firmware bundle from the project's [GitHub Releases](https://github.com/sheep-celica/pop-up-controller-v10/releases).

These release bundles are intended to be flashed using the [Pop-up Controller V10 Application](https://github.com/sheep-celica/Pop-up-controller-V10-Application).

The export script packages the files needed for ESP32 flashing:

- `bootloader.bin`
- `partitions.bin`
- `boot_app0.bin`
- `firmware.bin`

The repository also includes `scripts/export_flash_bundle.py` for generating a flashable bundle and an `esptool` command file.

## Serial Communication

The firmware exposes a serial command interface at `115200` baud, but in normal use that interface is mainly there so the [Pop-up Controller V10 Application](https://github.com/sheep-celica/Pop-up-controller-V10-Application) can communicate with the controller.

Manual serial commands do exist for diagnostics and configuration, but they are not the focus of this README. A separate command reference can live under `docs/` later.

One important note is that serial commands are only processed while the pop-ups are idle.

## Notes For People Browsing The Code

If you only want a quick overview:

- [src/main.cpp](src/main.cpp) contains startup flow and the main loop
- [include/config.h](include/config.h) contains project-wide configuration and pin assignments
- [src/services/pop_up_control](src/services/pop_up_control) contains the higher-level pop-up control logic
- [src/services/commands](src/services/commands) contains the serial command system
- [scripts](scripts) contains helper scripts for versioning and firmware export

## Support Expectations

This repository is primarily here as a reference for owners and tinkerers who want to understand or lightly customize the controller. It is not set up as a large collaborative open-source project, and there is no promise of published hardware design files at this stage.
