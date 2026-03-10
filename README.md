# pop-up-controller-v10
Firmware for the Pop-up controller board version 10
Remote control range 50 meters with antenna high. 30 meters with antenna under dash.

# Todo

# MAIN TODO
# MAIN TODO
# MAIN TODO

- do not sense when IDLE !
- why does the onboard blue LED turn on after HOLD-OFF but not upon startup when OFF -HOLD ????




- Save timing calibrations on shutdown      -DONE
- Automatic shutdown!!!                     -DONE
- Fix illumination                          -DONE
  - Seems to be On/OFF again                -DONE
  - Check for brightness change constantly  -DONE
- Error LED. Blink with stored errors upon startup  - DONE
- Log other errors too?
- Do stuff with debug button                                  - DONE
  - Toggle sleepy eye mode functionality with headlights ON   - DONE
  - print error codes                                         - DONE (actually just hold to restart)

- bug with wink pop up ending up UP instead of DOWN           - DONE
- a way to clear the timing calibrations                      - DONE
- sleepy eye mode TURN ON disabled with headlights on by default    - DONE

Debug btn
- 5 second hold = save memory and restart             - DONE
- 5 presses clear errors                              - DONE (just restarts tho. This will reset the pop-up state as well)
- 3 presses = sleepy eye mode with pop-ups allowed    - DONE
- 

commands
- wink RH                           - DONE
- wink LH                           - DONE
- wink both                         - DONE
- toggle both                       - DONE
- toggle sleepy eye mode  
- clearPopUptimingCalibration       - DONE

Sleepy eye mode toggle check for headlights on    - DONE


# Features done
- Pop-up UP/DOWN
- Pop-up wink
- Signal LED control
- Illumination control
- Pop-up braking
- ADS7138 integration
- Remote control

# Features TODO
- Error logging                     - DONE
- Battery voltage readout (with new revision) - DONE
- Idle turn off                     - DONE
- Current monitoring                
- Temperature monitoring            - DONE
- Statistics
  - Pop-up cycles                   - DONE
  - Boot cycles                     - DONE
  - Pop-up move time                - DONE
- Commands
  - Check statistics                - DONE
  - Check FW/manufacture date       - DONE
  - Clear errors                    - DONE
- Pop-up sleepy eye mode
  - go to inbetween position/reliable     - DONE (i think. gotta add hookup to the knob/button. Missing pullup)
  - auto calibrate based on voltage? Check if really required   - DONE
- Debug button

