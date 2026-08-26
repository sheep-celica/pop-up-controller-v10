# Changes Since Last Release

This file tracks unreleased changes on `main` and preserves the recent release history.

Current base release: `v2.1.0` (released 2026-08-15, commit `4957f57`)

## Unreleased

### Firmware

- Disabled firmware stall/overcurrent shutdown for all supported board builds while retaining DRV8243 hardware protections and separate motor-fault reporting.
- Remote inputs now trigger their assigned actions when pressed instead of when released.
- LED brightness potentiometer adjustments now apply immediately while illumination ON/OFF transitions retain their smooth fade.
- Sleepy-eye mode is now saved before sleep or shutdown and restored on startup, including its indicator LED, without moving either pop-up.

## Released

### `v2.1.0` - 2026-08-15

#### Firmware

- Added the protected, service-only `finalizeForShipment` workflow, which clears manufacturing-test errors, all statistics, and bench configuration while preserving manufacturing data and calibration, then enters read-only serial maintenance mode until power is disconnected.
- Added the `readPotValues` serial command so applications can read RH offset and LED-adjust potentiometer values while tuning the hardware.
- Fixed the Revision E PlatformIO board define so the normal Revision E environment selects the DRV8243 board configuration.
- Added `calibrateMotorCurrent rh|lh|both [duration_ms]` measurement output and `saveMotorCurrentCalibration <rh|lh> <scale> <offset_a>` for app-calculated, per-motor persisted current-sense corrections. Measurement duration remains configurable and RH/LH operate sequentially for a shared bench resistor.
- Added `testMotorCurrent [duration_ms]` for sequential RH/LH vehicle current checks with 100 ms calibrated-current samples and per-motor summaries.
- Added `printMotorCurrentCalibration` to report the active RH/LH current-sense corrections to the app.
- External remote-input expander disconnect detection is much faster, so a missing or unplugged expander no longer stalls the main loop for several seconds.
- Runtime remote-expander disconnect handling was smoothed further, making disconnect detection effectively unnoticeable during normal operation.
- Bench mode now still detects whether the external expander is connected for diagnostics, while remote inputs remain inactive in bench mode.
- The `readTemperature` serial command now prints `Not Connected` when the LM75 temperature sensor is missing, and startup logs a `TEMP_SENSOR_MALFUNCTION` error in that case.
- Toggle button X4 also winks when sleepy eye mode is ON
- Firmware now supports separate compile-time board targets for Pop-up Controller V10 Revision C and Revision E, with board-specific config headers and PlatformIO environments.
- `printBuildInfo` and startup logs now expose the compile-time board target so future app-side firmware selection can use a stable board identity.
- Added temporary Revision E bring-up firmware with an ADS7138 internal-expander startup check, avoiding the old POWER_ON latch path.
- Added a Revision E RH motor-current verification pass that samples before, during, and after a full-speed motor run.
- Added initial Revision E DRV8243 motor-driver infrastructure with safe-start duty handling and board-gated setup/update hooks.
- Added a pop-up motor adapter layer so shared pop-up movement logic can target either the Revision C `MotorController` path or the Revision E DRV8243 path.
- Added Revision E fault-expander input support so the BH and toggle buttons can use TCA6408A inputs while Revision C keeps direct ESP32 GPIOs.
- Added board capability flags and made power/latch helpers board-aware so Revision E does not use Revision C-only latch behavior.
- Renamed board pin configuration around power, wake, direct buttons, and DRV8243 sleep control so unavailable or repurposed pins are represented by their actual hardware function.
- Enabled the Revision E RH pop-up offset potentiometer on ADS7138 channel 2 and assigned the LED-adjust potentiometer to channel 3, matching the physical board labels.
- Added named Revision E fault-expander channel mappings and a service API for reading motor, sense, and illumination fault signals.
- Moved battery-voltage divider scaling into board-specific hardware config, preserving Revision C's 22k/2k scale and adding Revision E's 100k/10k scale.
- Restored startup battery-voltage readout, startup summary logging, and the shared bench-mode loop guard using board-specific battery scaling.
- Restored production input registration, debug-button registration, remote-input registration, and external expander runtime detection in the shared bring-up main.
- Restored runtime statistics updates and serial command processing in the shared bring-up main.
- Idle power-off serial commands now report unsupported board features explicitly on boards without idle power-off support, and `printEverything` reflects unsupported idle power-off and missing temperature sensors more clearly.
- Refactored temperature monitoring around hotspot and ambient sensor roles, preserving Revision C hotspot LM75 behavior and adding Revision E TMP112 ambient/hotspot support.
- Restored Revision C power-latch setup and idle power-off checks in the shared main, and Revision E now reuses the stored idle timeout for automatic deep sleep with GPIO 13 LOW wake-up outside bench mode.
- Added polling-based Revision E fault-expander reporting for motor, sensing, and illumination faults, plus a `readFaults` serial command.
- RH/LH motor and sensing faults now latch the affected pop-up motor disabled until `clearErrors` or a power cycle, matching the existing timeout recovery flow.
- Revision E now primes the shared DRV8243 `nSLEEP` wake/reset pulse during motor setup so startup fault polling does not latch false motor faults before the first movement.
- Digital LED control for the INPUT, STATUS, and ERROR LEDs is now polarity-aware per board revision, so shared LED logic works correctly on both Revision C and active-low Revision E hardware.
- Added persisted `printIlluminationFaultReporting` and `writeIlluminationFaultReporting <true|false>` serial commands so illumination fault reporting can be disabled on boards installed without external LED modules.
- Added a `forceSleep` serial command that immediately enters deep sleep on boards with deep-sleep wake support, such as Revision E.
- Switched the temporary Revision E bring-up firmware to an input-verification harness that registers normal inputs without updating pop-up motion.
- Removed the fixed 300 ms serial startup delay from the shared firmware startup path and renamed the remaining startup banner away from temporary integration-test wording.
- Added a `pop-up-controller-v10-rev-e-esp32-s3` PlatformIO target and board-config template for an ESP32-S3 Revision E variant using native USB CDC on GPIO19/GPIO20.
- Enabled the RH pop-up offset potentiometer and fault-expander interrupt GPIO in the Revision E ESP32-S3 board configuration.

#### Tooling

- The GitHub release workflow was updated to use Node 24 compatible action versions.
- Release bundle export now builds Rev C, Rev E, and Rev E ESP32-S3 firmware into one manifest-driven archive with per-board chip metadata and manual flashing commands.

#### Documentation

- Added manufacturing finalization documentation describing the preserved calibration/identity data, cleared runtime data, read-only inspection mode, and full-NVS erase command.
- Added a planning document for future multi-board firmware support using separate board-specific builds packaged into one app-consumable release archive.
- Added a board-aware firmware architecture note describing the shared-main, capability-flag, and low-level-adapter direction for multiple board configurations.
- Updated the multi-board firmware plan to reflect the initial Revision C/Revision E implementation scaffold and the recommended next steps.
- Added a Revision E hardware verification checklist for first-board bring-up.
- Added a Revision E main-porting checklist covering old-main behavior, Revision E power exclusions, and board-specific follow-up work.
- Expanded the Revision E porting checklist into a multi-board checklist covering board targets, PlatformIO environments, app identity, and multi-image release bundles.
- Clarified the Revision E porting checklist to keep fault-expander monitoring polling-based for now and to document side-specific latched motor-disable behavior for timeout, sense-fault, and motor-fault conditions.
- Updated build and flashing docs for multi-board PlatformIO environments, ESP32-S3 upload examples, and manifest-based release bundles.
