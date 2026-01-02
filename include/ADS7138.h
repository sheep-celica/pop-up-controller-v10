#ifndef ADS7138_H
#define ADS7138_H

#include <Arduino.h>
#include <Wire.h>

#ifndef ADS7138_DEFAULT_OVERSAMPLING
#define ADS7138_DEFAULT_OVERSAMPLING 16
#endif

class ADS7138 {
public:
    ADS7138(uint8_t i2c_addr = 0x10, double v_ref = 3.3);

    bool begin();

    // Pin configuration
    void set_pin_analog(uint8_t channel);
    void set_pin_digital_output(uint8_t channel, bool initial_level = LOW);
    void set_pin_digital_input(uint8_t channel);

    // Oversampling configuration
    void set_oversampling(uint8_t channel, uint8_t samples);

    // Read/write
    double read_analog(uint8_t channel);  // voltage with averaging
    void set_digital(uint8_t channel, bool level);
    bool read_digital(uint8_t channel);

private:
    uint8_t _i2c_addr;
    double _v_ref;

    bool _is_analog[8];
    bool _is_digital_output[8];
    bool _is_digital_input[8];

    uint8_t _oversampling[8];

    // I2C helpers
    void write_register(uint8_t reg, uint8_t value);
    uint8_t read_register(uint8_t reg);
    uint16_t read_channel_register(uint8_t channel);
};

#endif
