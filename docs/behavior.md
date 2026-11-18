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

The controller can accept optional remote commands from the legacy external
remote-input expander or, on Revision F ESP32-S3, from a paired EV1527 remote
through the CC1101 module. At boot, Revision F ESP32-S3 prefers a fully
connected CC1101 module and disables the legacy receiver; otherwise it falls
back to the legacy expander.

The four logical remote inputs provide:

- right-hand wink
- left-hand wink
- both-pop-up wink
- sleepy-eye mode
- remote inputs continue to be detected while either pop-up is moving; an
  individual pop-up ignores a new wink request while that pop-up is already busy
- the paired keyfob buttons map, in physical order, to EV1527 codes `0x8`,
  `0x4`, `0x2`, and `0x1`
- held wink buttons may request another wink after the previous movement ends;
  the sleepy-eye toggle is handled once per held press
- in bench mode, the controller can still detect whether the external expander is connected, but remote inputs stay inactive
- if the remote expander disconnects after being detected, the firmware logs `REMOTE_EXPANDER_DISCONNECTED` and ignores remote inputs until the next power cycle

## Sleepy-Eye Behavior

- sleepy-eye mode uses stored timing and live measurements to target an intermediate position
- the firmware continuously builds timing calibration data and uses it to better match the intermediate position of both pop-ups
- if the intermediate positions still do not match well enough, the on-board offset potentiometer can be adjusted to fine-tune the result
- sleepy-eye operation can be restricted while the headlights are active
- that safety behavior is configurable and stored persistently
- the active sleepy-eye mode is saved before sleep or shutdown and restored on startup
- restoring sleepy-eye mode leaves both motors idle at their existing physical positions and restores the indicator LED

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
