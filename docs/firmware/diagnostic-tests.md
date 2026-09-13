# Customer diagnostic tests

Run these tests only while the car is parked. The light-switch test retains normal pop-up control, so the lights may move as you operate the switch. The motor test moves the pop-ups automatically.

With the light switch fully **OFF**, press the internal debug button five times within 1.5 seconds. Both the raw UP and HOLD inputs must be inactive, and both debounced inputs must have been OFF for at least 200 ms. Presses while either input is active are ignored. The previous three-press setting toggle has been removed. A five-second debug-button hold while OFF still reboots the controller.

Illumination indicates diagnostic state at full PWM brightness, overriding the normal light-switch and potentiometer behavior:

| State | Illumination pattern |
| --- | --- |
| Select a test | Two short flashes every two seconds |
| Switch test | One second on, one second off |
| Motor test | Three short flashes every two seconds |
| Completed and saved | Steady on for three seconds |
| Aborted or save failed | Four short flashes every two seconds for three seconds |

In selection mode, RH starts the switch test, LH starts the motor test, and BH exits. RH/LH/BH, toggle, and sleepy-eye buttons do not perform their usual actions in diagnostic mode. Tests cannot be exited mid-run through the buttons; switch recording ends after 30 seconds, while motor testing ends on completion or a safety abort. When testing ends, the controller returns to selection mode. Press BH to leave.

For the **switch test**, after pressing RH, move the light switch through OFF → HOLD → UP → HOLD → OFF, pausing in each position. The controller records the initial raw and debounced UP/HOLD inputs, each observed input transition, and each pop-up movement request for 30 seconds. Normal switch control remains active. Raw transitions are sampled once per main loop; pulses shorter than a loop can be missed. Debounced transitions use the normal 30 ms input filter.

For the **motor test**, leave the switch OFF and press LH. The controller requires vehicle power, idle/unlocked motors, and sleepy-eye mode OFF. It runs RH UP–DOWN–UP, LH UP–DOWN–UP, then both UP–DOWN–UP, with one-second pauses between steps. Each target request, sensed position change, and step duration is recorded. The run aborts if the switch becomes active or a motor locks out. Existing motor timeouts and fault protection remain active. Normal switch commands are suppressed during this test and resume afterward.

Results are saved to a dedicated NVS namespace after each complete or aborted test; the newest record of each type replaces the older one. A power loss before the test ends can lose that run. Each record includes a test ID, firmware and board identity, duration, result, and an overflow flag. Each event has `ms` relative to test start, `kind`, `source`, `from`, `to`, and `duration_ms`. Source codes are UP input=1, HOLD input=2, RH=3, LH=4, both=5. Boolean values are 0/1; position and target values are DOWN=0, UP=1, IN_BETWEEN=2, IDLE=3, TIMEOUT=4. Abort reasons are switch active=1 and motor lockout=2. Event storage is capped at 160 events per test; `overflow=1` means later events were dropped.

After leaving diagnostic mode and taking the controller home, connect it to a PC at 115200 baud and send `printDiagnosticTests`. The app can parse `DIAG_BEGIN`, `DIAG_EVENT`, and `DIAG_END` lines. Send `clearDiagnosticTests` to remove the two saved records when they are no longer needed.
