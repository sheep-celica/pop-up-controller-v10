# Calibration

The controller stores several useful calibration and configuration values in non-volatile storage.

## Battery Voltage Calibration

Battery voltage readings can be calibrated with stored `a` and `b` constants so the reported battery value better matches the real system voltage.

## Pop-up Timing Calibration

The firmware can store pop-up timing data as a function of battery voltage.

That timing data is especially useful for sleepy-eye positioning, because the expected travel time changes with supply voltage.

## Persistent Runtime Configuration

The firmware also stores some behavior-related settings, including:

- idle auto-power-off timeout
- sleepy-eye safety behavior
- remote-input safety behavior
- remote-input pin mapping
- pop-up sensing and state-persistence timing values

## Where Calibration Data Lives

These values are stored in ESP32 non-volatile storage so they survive power cycles.

## Related Pages

- [Behavior](behavior.md)
- [Serial Commands](firmware/serial-commands.md)
- [Build and Flash](firmware/build-and-flash.md)
