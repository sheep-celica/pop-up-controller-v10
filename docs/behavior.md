# Behavior

This page gives a high-level overview of how the controller behaves in normal use.

## Factory Light Switch Behavior

- when the light-switch inputs indicate headlights up, both pop-ups are driven upward
- when the light-switch inputs return to the off state, both pop-ups are driven downward
- illumination control is tied into the light-switch behavior. HEAD position turns illumination on, otherwise it stays off

## Local Button Behavior

The controller firmware supports dedicated local button inputs mapped as follows:

- `X1`: right-hand wink
- `X2`: left-hand wink
- `X3`: both pop-ups wink
- `X4`: toggle behavior
- `X5`: sleepy-eye mode

## Optional Remote Inputs

If the external remote-input expander is connected, the controller can also accept optional remote commands for:

- right-hand wink
- left-hand wink
- both-pop-up wink
- sleepy-eye mode
- remote inputs are only polled while both pop-ups are idle or timed out
- in bench mode, the controller can still detect whether the external expander is connected, but remote inputs stay inactive
- if the remote expander disconnects after being detected, the firmware logs `REMOTE_EXPANDER_DISCONNECTED` and ignores remote inputs until the next power cycle

## Sleepy-Eye Behavior

- sleepy-eye mode uses stored timing and live measurements to target an intermediate position
- the firmware continuously builds timing calibration data and uses it to better match the intermediate position of both pop-ups
- if the intermediate positions still do not match well enough, the on-board offset potentiometer can be adjusted to fine-tune the result
- sleepy-eye operation can be restricted while the headlights are active
- that safety behavior is configurable and stored persistently

## Bench Mode

- when the detected supply voltage is below 7 V, the controller enters bench mode
- in bench mode, pop-up movement is disabled
- the main purpose of bench mode is to allow low-voltage flashing and setup without producing false pop-up timeout errors

## Persistent Settings

The controller stores settings and calibration data in non-volatile storage, including:

- behavior-related settings
- calibration values
- statistics
- manufacturing data

## Serial Communication

- the serial interface runs at `115200` baud
- it is mainly intended for communication with the application
- commands are only processed while the pop-ups are idle
