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
- Firmware now supports separate compile-time board targets for Pop-up Controller V10 Revision C and Revision D, with board-specific config headers and PlatformIO environments.
- `printBuildInfo` and startup logs now expose the compile-time board target so future app-side firmware selection can use a stable board identity.
- Added temporary Revision D bring-up firmware with an ADS7138 internal-expander startup check, avoiding the old POWER_ON latch path.
- Added a Revision D RH motor-current verification pass that samples before, during, and after a full-speed motor run.

### Tooling

- The GitHub release workflow was updated to use Node 24 compatible action versions.

### Documentation

- Added a planning document for future multi-board firmware support using separate board-specific builds packaged into one app-consumable release archive.
- Updated the multi-board firmware plan to reflect the initial Revision C/Revision D implementation scaffold and the recommended next steps.
- Added a Revision D hardware verification checklist for first-board bring-up.
