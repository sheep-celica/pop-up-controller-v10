# Board-Aware Firmware Architecture

This document describes the intended firmware structure for supporting multiple Pop-up Controller board configurations from one shared codebase.

The goal is to avoid one-off board forks while still keeping hardware differences explicit and safe.

## Scope

The firmware is expected to support several compile-time board targets, including:

- Revision C with the original ESP32 module
- Revision C with a newer ESP32 module
- Revision D with the original ESP32 module
- Revision D with a newer ESP32 module
- future board revisions such as Revision E

Each target should build as its own firmware image, selected by PlatformIO build environment and reported through compile-time board identity.

## Guiding Rules

Use one shared production `setup()` and one shared production `loop()` unless a future board genuinely needs a different runtime model.

Keep board-specific behavior behind board-aware services, lifecycle hooks, low-level adapters, and compile-time capability flags.

Keep pin maps, I2C addresses, ADC channels, input polarity, resistor-divider values, and feature availability in board configuration headers.

Prefer capability checks over revision checks in shared service code. Code should usually ask whether a feature exists, not whether the board is Revision C or Revision D.

Avoid duplicating high-level behavior between board variants. If two boards both support local inputs, pop-up movement, LEDs, runtime statistics, and serial commands, those flows should stay shared.

## Preferred Shape

The top-level firmware should look like one product flow:

```text
setup()
  setup_board_early()
  setup_power_or_sleep()
  Serial.begin(...)
  setup_i2c_bus()
  setup_io_expanders()
  setup_pop_ups()
  register_inputs()
  initialize_logging(...)
  setup_leds()
  setup_board_sensors()
  setup_startup_reporting()

loop()
  update_board_io()
  update_external_expander_runtime_state()
  update_inputs_and_motion_when_allowed()
  update_board_power_or_sleep()
  update_bench_mode_led_indicator(...)
  update_leds()
  statistics_manager.update_runtime()
  update_commands()
```

The exact function names can evolve, but the important point is that `main.cpp` should remain a readable orchestration layer. It should not accumulate board-specific branches for every hardware difference.

## Board Configuration

Board headers under `include/boards/` should define:

- stable board identity strings
- GPIO pin assignments
- I2C addresses and expander pin mappings
- ADC channels and analog scaling constants
- input polarity and electrical behavior
- board capability flags
- hardware-specific timing defaults where needed

The shared `include/board_config.h` selector should include exactly one board header based on the PlatformIO build environment.

## Capability Flags

Capability flags should describe hardware features and firmware support directly. They should be more specific than board revision names.

Examples:

```cpp
config::features::HAS_POWER_LATCH
config::features::HAS_DEEP_SLEEP_WAKE
config::features::HAS_DRV8243_MOTOR_DRIVER
config::features::HAS_FAULT_EXPANDER
config::features::HAS_EXTERNAL_REMOTE_EXPANDER
config::features::HAS_SINGLE_TEMPERATURE_SENSOR
config::features::HAS_DUAL_TEMPERATURE_SENSORS
```

These flags make it easier to support future combinations, such as a new ESP32 module on an existing PCB revision or a future board that shares one Revision D feature but not another.

Revision-specific checks are still acceptable in low-level construction code when selecting between incompatible concrete implementations, but shared behavior should prefer capabilities.

## Low-Level Adapters

Electrical differences should be hidden behind small adapters or normalization helpers.

Current example:

- `PopUpMotor` lets shared pop-up control use either the Revision C `MotorController` path or the Revision D DRV8243 path.
- `config::pins::POSITION_INPUT_ACTIVE_LOW` lets shared pop-up state reading normalize inverted UP/DOWN position inputs.

Expected future adapter areas:

- power management: Revision C power latch versus Revision D standby/deep sleep
- temperature: Revision C single sensor versus Revision D ambient and hotspot sensors
- battery voltage: shared high-level read API with board-specific divider constants
- fault handling: no-op or unavailable backend on boards without the fault expander
- input sources: direct GPIO input versus expander-backed input where the user-facing behavior is the same

Adapters should be boring. Their job is to make the shared service code read in terms of product behavior rather than PCB wiring.

## Main Loop Policy

The main loop should represent product behavior, not a list of board exceptions.

Shared loop behavior should remain shared:

- input updates
- pop-up movement updates
- remote input registration where supported
- LED updates
- runtime statistics
- serial command processing
- bench-mode indication
- board power or sleep maintenance

Board-specific services can decide whether a call does real work, logs an unsupported feature, or behaves as a no-op.

## Power And Sleep

Power behavior is the clearest place where boards differ.

Revision C uses the old power latch flow. Revision D does not support that latch flow and needs a standby or deep-sleep path instead.

The shared firmware should call a board-aware power service rather than calling Revision C-specific helpers directly from `main.cpp`.

Expected direction:

- Revision C backend wraps `setup_power()`, `power_on()`, `power_off()`, and `check_idle_time()`.
- Revision D backend exposes equivalent high-level intentions where possible, such as setup, idle handling, and shutdown or sleep requests.
- Serial commands and debug-button behavior should call the board-aware API so unsupported behavior can be handled clearly.

## Commands And Reporting

Serial commands should keep user-facing parsing and logs in command handlers, but any hardware action should go through the relevant service.

Commands that expose board-sensitive behavior should report unsupported features explicitly rather than silently doing nothing.

Firmware identity should continue to expose compile-time board identity so the desktop app can choose the correct firmware image from a multi-board release bundle.

## Documentation And Checklist Use

The multi-firmware release and packaging plan lives in `docs/firmware/board-revision-multi-firmware-plan.md`.

The tactical Revision D production-main work lives in `docs/firmware/revision-d-main-porting-checklist.md`.

This architecture document is the bridge between them: it describes how the code should be shaped while the checklist is being completed.
