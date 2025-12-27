# pop-up-controller-v10
Firmware for the Pop-up controller board version 10


# Todo
- Add motor breaking (N-Mos to pull motor pin LOW
- Update all components to be automotive grade (resist high voltage spikes ETC
- Safety stuff
  - i2c series resistors on SDA/SCL
  - i2c maybe small TVS diodes on SDA and SCL
- Transfer over safety stuff from the relay variant
- Try to separate the engine braking ground lines if possible
- Use TCA6408A Q1
- Make sure to not repeat the diode mistake (opposite placement)
