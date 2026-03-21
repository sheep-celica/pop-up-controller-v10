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
- `printEverything`: prints a broad status dump including manufacturing data, statistics, calibration, errors, temperature, and battery voltage
- `printBuildInfo`: prints firmware version and build timestamp
- `printStatisticalData`: prints stored statistics counters
- `printErrors`: prints the stored error log
- `readBatteryVoltage`: reads and prints the current battery voltage
- `readTemperature`: reads and prints the current temperature
- `getControllerStatus`: prints whether the controller is in `RUNNING` or `BENCH MODE`
- `getExternalExpander`: prints the detected external expander address or `Not Connected`
- `getIdleTimeToPowerOff`: prints the current idle auto-power-off timeout in seconds

## Motion And Interactive Control

- `wink <rh|lh|both>`: winks the selected pop-up or both pop-ups
- `toggle <both>`: toggles both pop-ups between up and down
- `toggleSleepyEyeMode`: toggles sleepy-eye mode on or off
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
- `printRemoteInputPins`: prints the current remote-input pin mapping
- `setRemoteInputPins <input1> <input2> <input3> <input4>`: remaps the four remote inputs to unique positions `1..4`
- `writeIdleTimeToPowerOffSeconds <seconds>`: sets the idle auto-power-off timeout

## Errors, Statistics, And Service Data

- `clearErrors`: clears stored error log entries, resets timeout state, and clears the error LED
- `clearStatisticalData <password>`: clears stored statistics after the required password is provided
- `writeManufactureData <manufacture_date> <serial_number> <board_serial> <board_revision> <car_model...>`: writes and locks one-time manufacturing data

## Notes

- command names, arguments, and responses are based on the current firmware
- many of these commands are primarily intended for application use rather than frequent manual entry
- this page can be expanded later with example responses and more precise behavior notes for each command
