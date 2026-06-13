# Revision D And Multi-Board Porting Checklist

This document tracks the work needed to move the current Revision D bring-up firmware back toward the normal production `main.cpp` behavior while also preparing the repository for multiple supported board configurations.

The near-term targets are Revision C and Revision D. The architecture should also leave room for additional combinations, such as old and new ESP32 module variants on the same PCB revision, and later hardware revisions such as Revision E.

Revision C and Revision D share most high-level firmware behavior, but several hardware paths are different enough that they should be handled through board-specific configuration, capability flags, board-aware service backends, or low-level adapters instead of one-off changes in `main.cpp`.

The architecture direction for this work is documented in [Board-Aware Firmware Architecture](board-aware-firmware-architecture.md). In short: keep one shared production `setup()` and one shared production `loop()`, and move board differences underneath the services they belong to.

## Goal

Refactor the firmware and release tooling so the repo can support multiple board designs without creating firmware forks.

Main goals:

- keep one shared production `setup()` and one shared production `loop()`
- build separate firmware images for each supported board target
- allow `platformio.ini` to carry board-specific settings where needed
- produce one release zip containing image files for every supported board target
- expose stable compile-time board identity so the external app can select the correct image
- restore full production firmware behavior for Revision C and Revision D

Production behaviors to bring back from the old main where they still apply:

- normal local input handling
- pop-up movement and timing behavior
- remote input registration and runtime detection
- LED updates
- runtime statistics
- serial command processing
- startup summaries and diagnostics

At the same time, avoid carrying over Revision C-only power behavior into Revision D.

## Current Bring-Up Main

The temporary Revision D test main currently focuses on:

- setting the shared motor sleep pin high early
- setting up serial logging
- setting up I2C
- setting up IO expanders
- setting up pop-up control
- setting up LEDs
- registering local inputs only
- updating inputs, pop-up control, and LEDs in the main loop

That is enough for input and pop-up validation, but it intentionally leaves out several production services.

## Board Compatibility Rules

- The new main should build and run for both Revision C and Revision D.
- Board targets should be selected at compile time, not detected dynamically at runtime.
- Each supported board target should have a stable board id and display name.
- Keep one shared production `setup()` and one shared production `loop()` unless a future board has a genuinely different runtime model.
- Shared behavior should stay shared where practical.
- Pin differences should remain in board config headers.
- PlatformIO environments may define different board, platform, partition, upload, or build-flag settings when hardware variants require them.
- Hardware capability differences should use capability flags, board-aware services, or low-level adapters.
- Prefer capability checks over direct Revision C/Revision D checks in shared service code.
- Avoid adding Revision D-only assumptions to shared code unless older boards are explicitly protected.

## Known Revision D Differences

### Power Behavior

Revision D does not support the old Revision C power latch flow.

Do not use these old power functions on Revision D:

- `power_on()`
- `power_off()`
- `setup_power()`
- `check_idle_time()`

Revision D needs a separate standby/deep-sleep behavior path instead. That behavior still needs to be designed and ported into the normal firmware flow.

### Temperature

Revision C uses the older temperature path.

Revision D uses two I2C temperature sensors:

- ambient temperature sensor
- hotspot temperature sensor

The Revision D sensors are a different, more precise type than the old sensor path. The production temperature service should become board-aware instead of assuming one shared sensor implementation.

### Battery Voltage

Battery voltage measurement is conceptually similar between board revisions, but Revision D has different resistor divider values.

The voltage conversion constants should come from board config so shared firmware can call the same high-level battery read API without hardcoding Revision C scaling.

### Inputs

Most input behavior should remain shared.

Known Revision D input differences:

- BH button uses the new IO expander instead of a direct ESP32 GPIO.
- Toggle button uses the new IO expander instead of a direct ESP32 GPIO.
- Other pin differences should already be handled by board config.

### Fault And Diagnostic Expander

Revision D has a new TCA6408A IO expander mostly used for FAULT/DIAG signals.

Revision D also has an interrupt pin for this expander. The firmware should eventually use that interrupt path for efficient fault/diagnostic state handling instead of relying only on polling.

### Position Sensing

Revision D position sensing has inverted UP/DOWN input polarity compared with Revision C.

This is currently handled by `config::pins::POSITION_INPUT_ACTIVE_LOW` and `normalize_position_input()` in `src/helpers/pop_up.cpp`.

## Porting Checklist

### Board Targets And Configuration

- [x] Add board-specific config headers for Revision C and Revision D.
- [x] Add `include/board_config.h` as the board selection layer.
- [x] Add stable compile-time board id and display name for Revision C.
- [x] Add stable compile-time board id and display name for Revision D.
- [ ] Decide how to name future old-ESP32 versus new-ESP32 board targets.
- [ ] Split board targets further if the new ESP32 module needs different PlatformIO settings, flash layout, bootloader behavior, or pin definitions.
- [x] Expand capability flags beyond the current Rev C/Rev D minimum.
- [x] Add capability flags for power latch, deep sleep wake, external remote expander, temperature sensor shape, and fault expander support.
- [x] Rename misleading shared pin names so unavailable or repurposed pins use functional names such as power latch, deep sleep wake, direct buttons, and motor-driver sleep.
- [x] Mark the RH pop-up offset potentiometer as unavailable on Revision D so its floating ADS7138 input is not configured or read.
- [ ] Move any remaining board-specific electrical constants out of shared config and into board config headers.

### PlatformIO Environments

- [x] Add a Revision C PlatformIO environment.
- [x] Add a Revision D PlatformIO environment.
- [ ] Confirm the default environment should remain Revision C during the transition.
- [ ] Add separate environments for old/new ESP32 variants if they need different PlatformIO `board`, `platform`, partition, upload, or build settings.
- [ ] Make sure every supported environment defines exactly one board-selection macro.
- [ ] Document the intended local build commands for every supported board target.

### Firmware Identity And App Contract

- [x] Expose compile-time board id and display name in build info.
- [ ] Decide which serial command response is the stable app-facing board identity contract.
- [ ] Keep manufacturing `board_revision` as traceability data rather than firmware-image selection data.
- [ ] Make the app-facing identity response include enough data for safe firmware selection.
- [ ] Document the exact board id strings the app should match.

### Main Loop Structure

- [ ] Replace temporary bring-up `main.cpp` with the shared production main shape.
- [ ] Add board-aware lifecycle hooks or services where `main.cpp` would otherwise need board-specific branches.
- [x] Restore local input updates.
- [x] Restore pop-up updates.
- [x] Restore LED updates.
- [x] Restore remote input registration/update flow where supported.
- [x] Restore runtime statistics updates.
- [x] Restore serial command processing.
- [x] Restore external expander runtime-state updates where supported.
- [x] Decide how bench mode should behave on Revision D.
- [x] Reintroduce the bench-mode loop guard in a board-compatible way.
- [x] Restore idle power-off loop checks where supported.

### Startup Flow

- [ ] Add a board-aware early setup hook for hardware that must be made safe before normal setup.
- [x] Initialize I2C.
- [x] Initialize IO expanders.
- [x] Initialize pop-up control.
- [x] Initialize LEDs.
- [x] Register local controls.
- [x] Replace temporary local-only input registration with production `register_inputs()` or a board-aware equivalent.
- [x] Restore startup battery voltage readout with board-specific scaling.
- [x] Restore startup summary logging.
- [x] Restore temperature setup using board-specific sensor support.
- [x] Decide final setup order for logging versus hardware setup.
- [x] Keep early Revision D motor sleep setup if required by DRV8243 startup behavior.

### Power And Sleep

- [x] Add a board-aware power/sleep service API used by `main.cpp`, debug-button behavior, and serial commands.
- [x] Keep old `setup_power()` behavior available through the Revision C power backend only.
- [x] Keep old `power_on()` behavior available through the Revision C power backend only.
- [x] Keep old `power_off()` behavior available through the Revision C power backend only.
- [x] Keep old `check_idle_time()` behavior available through the Revision C power backend only.
- [x] Restore `setup_power()` as the first setup call for Revision C and use the same board-aware entry point to initialize Revision D deep-sleep handling.
- [x] Restore `check_idle_time()` in the shared loop with Revision C power-latch shutdown behavior and Revision D deep-sleep behavior behind the same shared call.
- [x] Design Revision D standby/deep-sleep entry behavior.
- [x] Design Revision D wake behavior.
- [x] Decide how idle timeout should map to Revision D deep sleep.
- Revision D now reuses the stored idle timeout setting as an automatic deep-sleep threshold outside bench mode. On idle timeout it turns off LEDs and illumination, disables sensing and motors, drives the shared DRV8243 sleep pin LOW, and enters deep sleep with wake on `DEEP_SLEEP_WAKE_PIN` going LOW.
- [ ] Make debug-button power behavior board-aware.
- [ ] Make serial power/shutdown commands board-aware or disable them on Revision D.

### Temperature

- [x] Add a board-aware temperature service API that can represent one sensor, two sensors, or unsupported sensors.
- [x] Preserve Revision C temperature behavior.
- [x] Add Revision D ambient sensor support.
- [x] Add Revision D hotspot sensor support.
- [x] Decide how `readTemperature` should report one sensor versus two sensors.
- [x] Log missing/malfunctioning Revision D temperature sensors clearly.

### Battery And Bench Mode

- [x] Move battery divider values into board config if not already complete.
- [x] Verify Revision C battery voltage scaling still matches existing hardware.
- [x] Add/verify Revision D battery voltage scaling.
- [x] Decide whether Revision D bench mode uses the same voltage threshold.
- [x] Restore `initialize_controller_bench_mode(...)` once battery scaling is board-safe.
- [x] Restore `update_bench_mode_led_indicator(...)` once bench mode is board-safe.

### Inputs

- [x] Register Revision D local inputs.
- [x] Support BH button through the Revision D IO expander.
- [x] Support toggle button through the Revision D IO expander.
- [x] Restore debug button behavior.
- [x] Restore remote input 1-4 registration.
- [x] Restore remote input runtime detection.
- [ ] Confirm direct ESP32 input behavior still works for Revision C.
- [ ] Confirm expander-backed input behavior still works for Revision D.

### Fault And Diagnostic Handling

- [x] Add a capability flag for boards with the fault/diagnostic expander.
- [x] Verify Revision D fault expander connectivity.
- [x] Verify Revision D fault expander interrupt pin.
- [x] Verify Revision D fault/diagnostic input states.
- [x] Map Revision D fault expander channels for motor, sense, illumination, BH button, and toggle button inputs.
- [x] Add a named fault-signal read API for Revision D fault expander channels.
- [x] Add polling-based production fault reporting while the interrupt behavior remains undecided.
- [x] Add error codes for Revision D motor, sensing, and illumination fault signals.
- [x] Decide to keep production fault-expander handling polling-based for now and defer interrupt-driven handling.
- [x] Decide which FAULT/DIAG states should be logged.
- [x] Decide which FAULT/DIAG states should affect motor movement.
- [x] Implement side-specific latched motor-disable behavior for `RH_POP_UP_TIMEOUT`, `RH_SENSING_FAULT`, `RH_MOTOR_FAULT`, `LH_POP_UP_TIMEOUT`, `LH_SENSING_FAULT`, and `LH_MOTOR_FAULT` until power cycle or `clearErrors`.
- [x] Decide whether faults should be exposed through serial commands.
- [x] Add persisted serial-command control for disabling illumination fault reporting when external LED modules are not installed.

### Commands And Reporting

- [x] Restore `update_commands()`.
- [x] Audit commands that assume Revision C power behavior and route them through the board-aware power/sleep API.
- [x] Disable or adapt unsupported power commands through explicit unsupported-feature responses on Revision D.
- [x] Update build/status reporting if new Revision D sensor/fault states are exposed.
- [x] Update command docs when user-visible behavior changes.
- [x] Add `printIlluminationFaultReporting` and `writeIlluminationFaultReporting <true|false>` commands.

### Multi-Board Release Bundle

- [ ] Decide the final manifest filename and schema version.
- [ ] Extend `scripts/export_flash_bundle.py` or add a companion script that can build multiple PlatformIO environments.
- [ ] Stage each supported board target's image files in a board-specific folder.
- [ ] Include `bootloader.bin`, `partitions.bin`, `boot_app0.bin`, and `firmware.bin` for each board target where applicable.
- [ ] Write a manifest that maps board id to flash files, flash offsets, build version, and build timestamp.
- [ ] Validate the manifest against the staged files before creating the zip.
- [ ] Produce one app-consumable release zip containing all supported board targets.
- [ ] Update GitHub release automation to publish the multi-board zip.
- [ ] Document how the external app should choose the matching firmware image from the zip.

### Verification

- [x] Verify Revision D position input polarity fix.
- [x] Verify local inputs can move pop-ups to desired targets.
- [x] Build Revision C after each shared behavior port.
- [x] Build Revision D after each shared behavior port.
- [ ] Build every supported PlatformIO environment before release.
- [ ] Verify the multi-board release zip contains every supported board id.
- [ ] Verify the manifest paths and flash offsets match the packaged files.
- [ ] Run manual Revision C smoke test before treating main as production-safe.
- [ ] Run manual Revision D smoke test before treating main as production-safe.

## Old Main Behaviors To Port Back

The old production main included these setup calls:

- `setup_power()`
- `Serial.begin(115200)`
- `setup_i2c_bus()`
- `setup_io_expanders()`
- `setup_pop_ups()`
- `register_inputs()`
- `initialize_logging(...)`
- `setup_leds()`
- `read_battery_voltage()`
- `initialize_controller_bench_mode(...)`
- `log_startup_summary(...)`
- `setup_temperature()`

The old production loop included these calls:

- `update_external_expander_runtime_state()`
- `inputs_manager.update()`
- `update_pop_ups()`
- `update_remote_input_registration()`
- `check_idle_time()`
- `update_bench_mode_led_indicator(...)`
- `update_leds()`
- `statistics_manager.update_runtime()`
- `update_commands()`

Port these back unless they are explicitly unsupported on Revision D. Unsupported Revision D behavior should be replaced with a board-aware equivalent rather than silently dropped forever.

## Open Questions

- Should temperature commands preserve the old single-value output for Revision C and add a structured two-value output for Revision D?
- Should bench mode thresholds be shared or board-specific?
