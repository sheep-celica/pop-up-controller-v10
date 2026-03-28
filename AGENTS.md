# AGENTS.md

## Read This First

- This is firmware for a custom ESP32-based PCB that controls two pop-up headlight motors from several local and remote inputs.
- The main product target is the Pop-up Controller V10 board, primarily intended as a direct replacement for the factory light retractor relay in 5th generation Toyota Celica T18 models.
- Most changes in this repo are firmware behavior changes, hardware-integration changes, serial-command changes, or documentation updates for those areas.
- The repo root `README.md` is product-facing and high-level. Deeper technical context lives under `docs/`.

## Best Starting Context For A New Chat

If you need to get oriented quickly, read these in order:

1. `README.md`
2. `docs/firmware/architecture.md`
3. `src/main.cpp`
4. `include/config.h`
5. The service folder most related to the requested change

## Project Map

- `src/main.cpp`: setup order and main loop
- `include/config.h`: pin mapping, timing constants, NVS namespaces, thresholds, and feature configuration
- `src/helpers`: lower-level hardware helpers such as ADC access, motor control, braking, and pop-up primitives
- `src/services/pop_up_control`: high-level pop-up behavior and timing calibration persistence
- `src/services/inputs`: button, switch, knob, and remote-input registration and behavior
- `src/services/io`: LEDs, power-latching, and I/O expander setup
- `src/services/logging`: statistics, error log storage, manufacturing data, and logging helpers
- `src/services/commands`: serial command parser, command registry, and individual handlers
- `src/services/utilities`: battery voltage, temperature, controller status, and calibration utilities
- `docs/`: longer-form documentation split into hardware and firmware topics
- `scripts/`: version bumping and flash-bundle export helpers
- `test/`: currently only a placeholder README; there is no real unit test suite yet

## Runtime Mental Model

- `setup()` powers the board latch first, then brings up serial/I2C, expanders, pop-up control, inputs, logging, and LEDs.
- `loop()` updates inputs, pop-up behavior, LEDs, runtime statistics, serial commands, and idle power-off handling in that order.
- Bench mode is enabled below `config::utilities::BENCH_MODE_MAX_BATTERY_V` and disables pop-up movement.
- Serial commands are only processed while both pop-ups are idle or timed out.
- Persistent state is spread across several NVS namespaces, so storage-related changes should be made carefully.

## Working Style For This Repo

- Prefer small, localized changes that fit the existing service boundaries.
- Keep parsing and user-facing command logs in command handlers, and keep business logic in the relevant service modules.
- Keep hardware constants, timing defaults, pin assignments, and persistent namespace names in `include/config.h` unless there is a strong reason not to.
- If a change touches pop-up movement, also inspect input triggers, safety behavior, and any persisted calibration that affects motion timing.
- If a change touches persistence, inspect every service that owns related `Preferences` state before changing erase, reboot, or migration behavior.
- Update docs when behavior visible to users, installers, or service tooling changes.

## Release Change Tracking

- Keep `docs/changes-since-last-release.md` updated with short bullet points for changes made after the latest tagged release.
- Add brief descriptions only; focus on user-visible behavior, hardware integration, serial-command, documentation, and release-tooling changes.
- When a new release is cut, roll the file forward so it tracks changes since that new release.

## Build And Verification

- Preferred build command in this environment:
  `C:\Users\Sheep\.platformio\penv\Scripts\platformio.exe run`
- Useful upload command:
  `C:\Users\Sheep\.platformio\penv\Scripts\platformio.exe run -t upload`
- Useful monitor command:
  `C:\Users\Sheep\.platformio\penv\Scripts\platformio.exe device monitor -b 115200`
- Since there is no real automated test suite yet, the normal baseline verification is a successful build plus targeted manual or serial smoke testing.

## Serial Command Workflow

When adding or changing a serial command:

1. Implement the handler in its own file under `src/services/commands/definitions/`.
2. Add the extern declaration to `include/services/commands/command_definitions.h`.
3. Register the command in `src/services/commands/commands_registry.cpp`.
4. Keep the command name and usage in `camelCase`, matching the existing command set.
5. Validate arguments inside the handler and log a `Usage: ...` line when the call shape is wrong.
6. Keep the handler focused on parsing, validation, and logs; move reusable behavior into the proper service module when it starts becoming real business logic.

## Command Visibility And Follow-Ups

- Public commands use the default `visible_in_help = true`.
- Hidden or service-only commands must set `visible_in_help = false` in their `CommandDefinition`.
- Only public commands should be listed in `docs/firmware/serial-commands.md`.
- If a command needs a second line of input, use `request_next_command_line(...)` from `include/services/commands/commands.h`.
- That callback receives the next trimmed serial line, or `timed_out = true` if no line arrives before the timeout.
- While a follow-up line is pending, the next line is not parsed as a normal command.

## Destructive NVS Changes

- `reboot_controller()` and `power_off()` save state back into NVS before restart or shutdown.
- If the goal is to wipe NVS, do not call those helpers after the erase step or the firmware can immediately repopulate stored data.
- For a true full-NVS erase flow, flush serial output and reboot directly with `ESP.restart()`.
- Reuse the shared destructive-command password from `include/services/commands/command_passwords.h` when a new protected clear flow should behave like the existing ones.

## Good Files To Inspect Before Editing Specific Areas

- Motion behavior: `src/services/pop_up_control/pop_up_control.cpp`
- Timing calibration: `src/services/pop_up_control/pop_up_timing_calibration.cpp`
- Input behavior: `src/services/inputs/register_inputs.cpp` and `src/services/inputs/logic/*`
- Power and reboot behavior: `src/services/io/power.cpp`
- Persistence, statistics, and manufacture data: `src/services/logging/logging.cpp` and `src/services/logging/statistics_manager.cpp`
- Serial command system: `src/services/commands/commands.cpp`, `src/services/commands/commands_registry.cpp`, and `src/services/commands/definitions/*`

## Notifications

- Before sending your final user-facing response for a completed task, run `C:\Users\Sheep\AppData\Local\Python\pythoncore-3.14-64\python.exe .codex\tools\notify_client.py "Codex finished" "Task complete in this repository."` from the repository root to send the desktop notification.
- When you are blocked waiting for my approval or input, run `C:\Users\Sheep\AppData\Local\Python\pythoncore-3.14-64\python.exe .codex\tools\notify_client.py "Codex needs input" "Waiting for approval or more instructions."` from the repository root to send the desktop notification.
- If the notification command fails once, continue with the task and mention briefly that the notification failed.
