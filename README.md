# pop-up-controller-v10
Firmware for the Pop-up controller board version 10
Remote control range 50 meters with antenna high. 30 meters with antenna under dash.

# Todo

# MAIN TODO
# MAIN TODO
# MAIN TODO
- Save timing calibrations on shutdown      -DONE
- Automatic shutdown!!!                     -DONE
- Fix illumination                          -DONE
  - Seems to be On/OFF again                -DONE
  - Check for brightness change constantly  -DONE
- Error LED. Blink with stored errors upon startup
- Log other errors too?
- Do stuff with debug button
  - Toggle sleepy eye mode functionality with headlights ON
  - print error codes



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

