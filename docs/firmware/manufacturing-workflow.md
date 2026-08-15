# Manufacturing Finalization

The firmware provides two hidden, service-only destructive commands for bench and manufacturing work. They are protected by the shared destructive-command password.

## Finalize a board for shipment

Run:

```text
finalizeForShipment <password>
```

This command is intended to run after manufacturing data and all electrical and pop-up calibrations have been saved.

It preserves:

- locked manufacturing data
- battery-voltage calibration
- motor-current calibration
- RH/LH pop-up timing calibration

It clears:

- the stored error log
- all statistical counters, including boot, runtime, cycle, movement, input, and RH/LH error counters
- bench configuration for fault reporting, remote inputs, sleepy-eye behavior, pop-up runtime tuning, and idle shutdown
- latched runtime motor faults and the error LED

After completion, the firmware enters a RAM-only read-only maintenance mode. Runtime input handling, pop-up updates, fault polling, LED updates, statistics updates, automatic shutdown, reboot, power-off, and write commands are disabled. Read-only serial commands remain available for final inspection, including `printEverything`, calibration print commands, `printErrors`, and `printStatisticalData`.

The command does not power the board off. Disconnect the board's power manually after inspection. This prevents the normal shutdown path from writing any cleared runtime state back to NVS.

The maintenance mode is not persisted. A reboot returns the firmware to normal operation with the cleaned NVS state.

## Erase all NVS

The separate hidden `clearAllNvs <password>` command erases the complete NVS partition, including manufacturing data and all calibration data, then reboots. It is intended only for service or development use.
