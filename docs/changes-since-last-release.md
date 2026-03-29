# Changes Since Last Release

This file tracks unreleased changes on `main` after the most recent tagged release.

Current base release: `v1.1.0`

## Unreleased

### Firmware

- External remote-input expander disconnect detection is much faster, so a missing or unplugged expander no longer stalls the main loop for several seconds.
- Runtime remote-expander disconnect handling was smoothed further, making disconnect detection effectively unnoticeable during normal operation.
- Bench mode now still detects whether the external expander is connected for diagnostics, while remote inputs remain inactive in bench mode.
- The `readTemperature` serial command now prints `Not Connected` when the LM75 temperature sensor is missing, and startup logs a `TEMP_SENSOR_MALFUNCTION` error in that case.
- Toggle button X4 also winks when sleepy eye mode is ON

### Tooling

- The GitHub release workflow was updated to use Node 24 compatible action versions.
