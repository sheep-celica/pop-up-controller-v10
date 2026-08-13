# Serial Commands

The controller exposes a serial command interface at `115200` baud.

In normal use, this interface is mainly there so the [Pop-up Controller V10 Application](https://github.com/sheep-celica/Pop-up-controller-V10-Application) can communicate with the controller.

## Important Behavior

- commands are line-based
- commands are only processed while the pop-ups are idle
- the interface is useful for diagnostics, configuration, and manufacturing/service tasks
- `help` prints the currently registered command list

## Information And Status

- `help`: prints the full list of available commands
- `printEverything`: prints a broad status dump including manufacturing data, statistics, calibration, errors, temperature, battery voltage, board-supported power features, and fault-reporting configuration
- `printBuildInfo`: prints firmware version, build timestamp, board id, and board display name
- `printStatisticalData`: prints stored statistics counters
- `printErrors`: prints the stored error log
- `readBatteryVoltage`: reads and prints the current battery voltage
- `readTemperature [hotspot|ambient|all]`: reads temperature by sensor role. With no argument, reads the hotspot sensor for compatibility. Unsupported or missing sensors are reported explicitly.
- `readFaults`: prints the current fault-expander state, including unsupported, disconnected, inactive, active, and reporting-disabled fault states
- `getControllerStatus`: prints whether the controller is in `RUNNING` or `BENCH MODE`
- `getExternalExpander`: prints the detected external expander address or `Not Connected`
- `getIdleTimeToPowerOff`: prints the current idle shutdown timeout in seconds. Revision C uses it for auto power-off, while Revision E uses it for automatic deep sleep.

## Motion And Interactive Control

- `wink <rh|lh|both>`: winks the selected pop-up or both pop-ups
- `toggle <both>`: toggles both pop-ups between up and down
- `toggleSleepyEyeMode`: toggles sleepy-eye mode on or off
- `forceSleep`: immediately enters deep sleep on boards that support it, such as Revision E. Unsupported boards reject the command.
- `reboot`: reboots the controller

## Calibration

- `printBatteryVoltageCalibration`: prints the stored battery-voltage calibration constants
- `writeBatteryVoltageCalibration <a> <b>`: stores new battery-voltage calibration constants
- `printPopUpTimingCalibration [rh|lh|both]`: prints the stored pop-up timing calibration table for one or both sides
- `savePopUpTimingCalibration [rh|lh|both]`: forces the current timing calibration table to be saved to NVS
- `clearPopUptimingCalibration`: clears both stored pop-up timing calibration tables

## Runtime Configuration

- `printPopUpMinStatePersistMs`: prints the current pop-up state-persistence filter value
- `writePopUpMinStatePersistMs <milliseconds>`: sets the pop-up state-persistence filter value
- `printPopUpSensingDelayUs`: prints the current sensing settle delay
- `writePopUpSensingDelayUs <microseconds>`: sets the sensing settle delay used before reading pop-up state
- `printSleepyEyeModeWithHeadlights`: prints whether sleepy-eye mode is allowed while headlights are active
- `writeSleepyEyeModeWithHeadlights <true|false>`: allows or blocks sleepy-eye mode while headlights are active
- `printRemoteInputsWithHeadlights`: prints whether remote inputs are allowed while headlights are active
- `writeRemoteInputsWithHeadlights <true|false>`: allows or blocks remote inputs while headlights are active
- `printIlluminationFaultReporting`: prints whether illumination fault reporting is enabled
- `writeIlluminationFaultReporting <true|false>`: enables or disables illumination fault reporting for boards with the fault expander
- `printRemoteInputPins`: prints the current remote-input pin mapping
- `setRemoteInputPins <input1> <input2> <input3> <input4>`: remaps the four remote inputs to unique positions `1..4`
- `writeIdleTimeToPowerOffSeconds <seconds>`: sets the idle shutdown timeout. Revision C uses it for auto power-off, while Revision E uses it for automatic deep sleep.

## Errors, Statistics, And Service Data

- `clearErrors`: clears stored error log entries, clears latched timeout/fault movement lockouts, and clears the error LED
- `calibrateMotorCurrent <rh|lh|both> [duration_ms]`: measures the selected Revision E motor current channels without saving corrections, prints uncalibrated 100 ms samples, and emits `MOTOR_CAL_RESULT` data for the app. The default measurement duration is 1500 ms; the optional duration applies per motor. The `both` workflow runs RH and LH sequentially so one shared resistor can remain connected.
- `saveMotorCurrentCalibration <rh|lh> <scale> <offset_a>`: applies and persists one app-calculated current-sense correction immediately. The command does not activate a motor.
- `printMotorCurrentCalibration`: prints the currently active RH/LH scale and offset values without activating a motor.
- `testMotorCurrent [duration_ms]`: runs RH and LH sequentially and prints calibrated current samples every 100 ms plus per-motor summaries. The default duration is 2000 ms per motor.
- `clearStatisticalData <password>`: clears stored statistics after the required password is provided
- `writeManufactureData <manufacture_date> <serial_number> <board_serial> <board_revision> <car_model...>`: writes and locks one-time manufacturing data
- `writeManufactureData` converts `_` to spaces in the stored manufacturing-data fields before saving to NVS

## Notes

- command names, arguments, and responses are based on the current firmware
- many of these commands are primarily intended for application use rather than frequent manual entry
- this page can be expanded later with example responses and more precise behavior notes for each command
