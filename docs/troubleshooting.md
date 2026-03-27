# Troubleshooting

This page collects a few common things to check before digging into the source code.

## Controller Powers Up But Pop-ups Do Not Move

Check whether the controller has entered bench mode.

- bench mode is triggered when the measured supply voltage is below 7 V. The STATUS and INPUT LEDs would be flashing rapidly
- in bench mode, pop-up motion is intentionally disabled

## Remote Inputs Do Not Work

The optional remote-input feature depends on the external I/O expander being detected correctly.

If that hardware is not connected, the firmware skips remote-input registration and keeps retrying while both pop-ups are idle.

If you connect the expander after boot, the firmware can register the remote inputs during runtime once both pop-ups are idle.

In bench mode, the controller can still detect the external expander for diagnostics such as `getExternalExpander`, but remote inputs themselves stay inactive.

If a previously detected expander disconnects at runtime, the firmware logs `REMOTE_EXPANDER_DISCONNECTED`, disables remote inputs, and keeps them disabled until the controller is power-cycled.

## Serial Commands Or App Requests Do Not Respond

Serial commands are only processed while the pop-ups are idle.

If the controller is currently moving a pop-up, wait until it returns to an idle state and try again.

## Need More Detail?

- [Behavior](behavior.md)
- [Calibration](calibration.md)
- [Serial Commands](firmware/serial-commands.md)
- [Application](application.md)
