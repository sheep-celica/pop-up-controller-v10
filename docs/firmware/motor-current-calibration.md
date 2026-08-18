# Motor Current Calibration

The `calibrateMotorCurrent` command measures the Revision E DRV8243 IPROPI readout for one or both motor channels. It reports uncalibrated readings; the app supplies the resistor measurement and saves the calculated correction separately.

## Bench Setup

1. Use a current-limited supply and verify the voltage across the resistor while each motor is active.
2. Disconnect the mechanical motors.
3. Connect the known load resistor from the selected driver output to GND as intended by the board wiring.
4. Use a voltmeter directly across the resistor. An ammeter is optional, but it adds series resistance and is not required when the resistor value is known.
5. Keep the resistor cooled and within its continuous power rating. A 4 ohm load at 10.8 V is approximately 2.7 A and 29 W.

## Command

Use one of:

```text
calibrateMotorCurrent rh
calibrateMotorCurrent lh
calibrateMotorCurrent both
calibrateMotorCurrent both 4000
```

Each channel records a zero-current baseline, runs at 100% duty for 1.5 seconds by default, prints uncalibrated samples every 100 ms, and emits one machine-readable result line. The optional second argument sets the active measurement duration in milliseconds per motor, so `4000` means 4 seconds. This command does not save corrections.

For `both`, the firmware calibrates RH and LH sequentially. A single shared resistor can remain connected if the inactive DRV8243 output is in its normal coast/disabled Hi-Z state. Do not tie complete H-bridge output pairs together; use the intended output-to-ground load connection for each channel.

For each motor, the app calculates the reference current from:

```text
reference current = measured resistor voltage / resistor resistance
```

The app then calculates and saves the correction:

```text
scale = reference current / delta_a
offset_a = -zero_a * scale
```

Use this command once for each motor:

```text
saveMotorCurrentCalibration <rh|lh> <scale> <offset_a>
```

Example:

```text
saveMotorCurrentCalibration rh 1.42 -0.003
```

The save command applies the correction immediately and persists it in the `motor_cal` NVS namespace. It does not activate a motor.

To read the currently active corrections:

```text
printMotorCurrentCalibration
```

The firmware responds with one line per motor:

```text
MOTOR_CAL_CONFIG motor=RH status=ok scale=1.405000 offset_a=-0.002000
MOTOR_CAL_CONFIG motor=LH status=ok scale=1.410000 offset_a=-0.001500
```

These are the values currently applied by the firmware, including values loaded from NVS at boot or saved during the current session.

Successful measurement output has this form:

```text
MOTOR_CAL_RESULT motor=RH status=ok duration_ms=4000 zero_a=0.000000 active_a=1.925000 delta_a=1.925000 samples=40
```

Firmware stall/overcurrent shutdown is disabled in the current product configuration. The calibration routine still temporarily disables the detector and restores its prior state before reporting the result. The DRV8243 hardware protections remain active.

After calibration, use the normal motor movement workflow and confirm that the reported current is reasonable before selecting a final stall threshold.

## Vehicle Current Check

After installing the module in the vehicle, use:

```text
testMotorCurrent
```

This runs RH and LH sequentially for 2 seconds each and logs calibrated current samples every 100 ms. An optional duration in milliseconds applies to each motor:

```text
testMotorCurrent 3000
```

Machine-readable samples have this form:

```text
MOTOR_CURRENT_TEST_SAMPLE motor=RH t_ms=100 raw=380 current_a=2.710
```

Each motor ends with a summary:

```text
MOTOR_CURRENT_TEST_RESULT motor=RH status=ok duration_ms=2000 samples=20 average_a=2.700 min_a=2.650 max_a=2.760
```

The test is supported on the DRV8243-based Revision E path. Revision C has no implemented current-readout conversion in its legacy motor-controller path.
