# Major Components

This page lists the more important components currently called out in the public documentation.

## Main Controller And Sensing

- `ESP32 DevKit V1`: main microcontroller platform that runs the firmware
- `ADS7138`: internal I/O expander and ADC used for on-board analog and digital signals
- `PCF8574`: optional external I/O expander used for remote inputs
- `LM75`: board-temperature sensor

## Motor Control

- `IX4427N` from IXYS: MOSFET driver for the braking NMOS stage
- `Infineon IPD70N10S3-12`: pop-up braking NMOS
- `BTS6163D`: power switch for the pop-up motors

## Power And Protection

- `Vishay SM8S18AHE3_A/I`: main TVS protection diode
- `Panasonic EEUFR1V471B`: power-input bulk capacitor

## Notes

This is not yet intended to be a full BOM. It is just a public-facing summary of the more important parts currently mentioned in the repo documentation.
