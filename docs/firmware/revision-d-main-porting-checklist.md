# Revision D Main Porting Checklist

This document tracks the work needed to move the current Revision D bring-up firmware back toward the normal production `main.cpp` behavior while keeping the firmware compatible with both Revision C and Revision D builds.

Revision C and Revision D share most high-level firmware behavior, but several hardware paths are different enough that they should be handled through board-specific configuration or board-gated service code instead of one-off changes in `main.cpp`.

## Goal

Bring back the production behaviors from the old main where they still apply:

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
- Shared behavior should stay shared where practical.
- Pin differences should remain in board config headers.
- Hardware capability differences should use board-specific config flags or small board-gated service branches.
- Avoid adding Revision D-only assumptions to shared code unless Revision C is explicitly protected.

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

### Main Loop Structure

- [x] Restore local input updates.
- [x] Restore pop-up updates.
- [x] Restore LED updates.
- [ ] Restore remote input registration/update flow where supported.
- [ ] Restore runtime statistics updates.
- [ ] Restore serial command processing.
- [ ] Restore external expander runtime-state updates where supported.
- [ ] Decide how bench mode should behave on Revision D.
- [ ] Reintroduce the bench-mode loop guard in a board-compatible way.

### Startup Flow

- [x] Initialize I2C.
- [x] Initialize IO expanders.
- [x] Initialize pop-up control.
- [x] Initialize LEDs.
- [x] Register local controls.
- [ ] Replace temporary local-only input registration with production `register_inputs()` or a board-aware equivalent.
- [ ] Restore startup battery voltage readout with board-specific scaling.
- [ ] Restore startup summary logging.
- [ ] Restore temperature setup using board-specific sensor support.
- [ ] Decide final setup order for logging versus hardware setup.
- [ ] Keep early Revision D motor sleep setup if required by DRV8243 startup behavior.

### Power And Sleep

- [ ] Keep old `setup_power()` enabled for Revision C only.
- [ ] Keep old `power_on()` behavior enabled for Revision C only.
- [ ] Keep old `power_off()` behavior enabled for Revision C only.
- [ ] Keep old `check_idle_time()` behavior enabled for Revision C only.
- [ ] Design Revision D standby/deep-sleep entry behavior.
- [ ] Design Revision D wake behavior.
- [ ] Decide how idle timeout should map to Revision D deep sleep.
- [ ] Make debug-button power behavior board-aware.
- [ ] Make serial power/shutdown commands board-aware or disable them on Revision D.

### Temperature

- [ ] Preserve Revision C temperature behavior.
- [ ] Add Revision D ambient sensor support.
- [ ] Add Revision D hotspot sensor support.
- [ ] Decide how `readTemperature` should report one sensor versus two sensors.
- [ ] Log missing/malfunctioning Revision D temperature sensors clearly.

### Battery And Bench Mode

- [ ] Move battery divider values into board config if not already complete.
- [ ] Verify Revision C battery voltage scaling still matches existing hardware.
- [ ] Add/verify Revision D battery voltage scaling.
- [ ] Decide whether Revision D bench mode uses the same voltage threshold.
- [ ] Restore `initialize_controller_bench_mode(...)` once battery scaling is board-safe.
- [ ] Restore `update_bench_mode_led_indicator(...)` once bench mode is board-safe.

### Inputs

- [x] Register Revision D local inputs.
- [x] Support BH button through the Revision D IO expander.
- [x] Support toggle button through the Revision D IO expander.
- [ ] Restore debug button behavior.
- [ ] Restore remote input 1-4 registration.
- [ ] Restore remote input runtime detection.
- [ ] Confirm direct ESP32 input behavior still works for Revision C.
- [ ] Confirm expander-backed input behavior still works for Revision D.

### Fault And Diagnostic Handling

- [x] Verify Revision D fault expander connectivity.
- [x] Verify Revision D fault expander interrupt pin.
- [x] Verify Revision D fault/diagnostic input states.
- [ ] Add production handling for the fault expander interrupt.
- [ ] Decide which FAULT/DIAG states should be logged.
- [ ] Decide which FAULT/DIAG states should affect motor movement.
- [ ] Decide whether faults should be exposed through serial commands.

### Commands And Reporting

- [ ] Restore `update_commands()`.
- [ ] Audit commands that assume Revision C power behavior.
- [ ] Disable or adapt unsupported power commands for Revision D.
- [ ] Update build/status reporting if new Revision D sensor/fault states are exposed.
- [ ] Update command docs when user-visible behavior changes.

### Verification

- [x] Verify Revision D position input polarity fix.
- [x] Verify local inputs can move pop-ups to desired targets.
- [ ] Build Revision C after each shared behavior port.
- [ ] Build Revision D after each shared behavior port.
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

- What should the final Revision D deep sleep API look like?
- Should Revision D idle timeout enter deep sleep automatically?
- What exact wake sources should Revision D enable in production firmware?
- Should Revision D fault expander interrupts only count/log events, or should some faults immediately affect motor behavior?
- Should temperature commands preserve the old single-value output for Revision C and add a structured two-value output for Revision D?
- Should bench mode thresholds be shared or board-specific?
