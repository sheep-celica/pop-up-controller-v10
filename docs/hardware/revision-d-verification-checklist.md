# Revision D Verification Checklist

Track first-board bring-up checks for the Pop-up Controller V10 Revision D hardware.

## ADS7138 Internal IO Expander

- [x] Check connectivity.
- [x] Check LED control.
- [x] Check analog reads.
- [ ] Check debug button.
- [ ] Check sleepy LED.

## Temperature Sensors

- [ ] Check ambient sensor connectivity. Verification code assumes address `0x48` (`ADD0` to GND).
- [x] Check hotspot sensor connectivity. Verification code assumes address `0x49` (`ADD0` to VCC).
- [x] Verify temperature readout.

## Illumination

- [ ] Verify PWM dimming.
- [ ] Verify short-circuit protection.
- [ ] Verify diagnostic pin feedback.

## Positioning

- [x] Verify DOWN position readout.
- [x] Verify UP position readout.
- [x] Verify sensing circuit control.
- [ ] Verify sensing circuit diagnostic feedback.

## Wake-Up

- [x] Verify deep sleep with wake-up pin.

## Buttons

- [x] Verify all 5 buttons can be registered.

## Light Switch

- [x] Verify light-switch sensing circuits.

## Motors

- [x] Verify ON/OFF control.
- [x] Verify PWM control.
- [x] Verify sleep.
- [x] Verify RH motor current readout during full-speed run.

## Standby Circuit

- [x] Verify standby puts the board to sleep.
- [x] Verify all circuits can wake the board from standby.
- [x] Verify motor ICs wake up with a special wake-up pulse.

## Fault IO Expander

- [x] Verify connection.
- [x] Verify interrupt pin.
- [x] Verify all fault pins work.
- [x] Verify auxiliary BTN3 and BTN4 inputs work.
