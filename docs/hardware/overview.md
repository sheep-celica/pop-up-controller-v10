# Hardware Overview

The Pop-up Controller V10 board is designed primarily as a direct replacement for the factory Light Retractor Relay used in Toyota Celica T18 models.

![Pop-up Controller V10 PCB](../assets/images/controller_pcb.png)

## High-Level Functional Blocks

- ESP32 DevKit V1 for the main control firmware
- internal analog and digital I/O handled through the ADS7138
- optional remote input support through a PCF8574-based external expander
- Revision F ESP32-S3 support for an external remote module with a PCA9536 troubleshooting I/O expander and CC1101 sub-1 GHz radio
- temperature monitoring via an LM75-compatible sensor
- dedicated motor-control and protection hardware for the pop-up motors
- illumination, button, switch, and vehicle-facing input handling

The Revision F ESP32-S3 remote module uses PCA9536 P0 for its active-low pair button and P1-P3 for active-high pair, status, and debug LEDs. The CC1101 uses GPIO42 SCLK, GPIO44 CS, GPIO2 MISO/GDO1, and GPIO1 MOSI. The original Revision F routing connects GDO0 to the shared wake rail through diode logic. The validated prototype bodge and planned board revision instead connect GDO2 to that rail while retaining its active-low carrier-activity LED; GDO0 remains high impedance in this configuration.

On Revision F ESP32-S3 the firmware selects one remote-input receiver during
boot. A connected PCA9536 and CC1101 pair takes priority; when present, the
legacy PCF8574 remote-input expander is not initialized or polled. If the new
module is unavailable, firmware falls back to the legacy expander.

Revision F ESP32-S3 can use two-stage remote Wake-on-Radio sleep. The initial stage uses an approximately -60 dBm carrier threshold for better wake range and retains an RTC-backed deadline across unqualified wakes. After the configured interval, firmware changes to the approximately -54 dBm long-term profile to suppress interference. A wake with no held physical input remains awake for a configurable evaluation interval so a continued paired EV1527 transmission can qualify it. The feature, initial-stage duration, and evaluation duration are configured in the Revision F ESP32-S3 board header.

## Current Documentation Scope

- this repo documents the firmware-facing view of the hardware
- schematics are not published here at this time
- connector-level installation details are intentionally kept out of the source repo for now

## Related Pages

- [Major Components](major-components.md)
- [I/O and Connectors](io-and-connectors.md)
