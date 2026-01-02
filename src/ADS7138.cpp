#include "ADS7138.h"

#define REG_PIN_CFG     0x02
#define REG_GPIO_CFG    0x03
#define REG_GPO_DRIVE   0x04
#define REG_GPO_VALUE   0x05
#define REG_GPI_VALUE   0x06
#define REG_CONVERSION  0x08

ADS7138::ADS7138(uint8_t i2c_addr, double v_ref) {
    _i2c_addr = i2c_addr;
    _v_ref = v_ref;

    for (uint8_t i = 0; i < 8; i++) {
        _is_analog[i] = true;
        _is_digital_output[i] = false;
        _is_digital_input[i] = false;
        _oversampling[i] = ADS7138_DEFAULT_OVERSAMPLING;
    }
}

bool ADS7138::begin() {
    Wire.begin();

    uint8_t pin_cfg = 0;
    uint8_t gpio_cfg = 0;
    uint8_t gpo_drive = 0;

    for (uint8_t i = 0; i < 8; i++) {
        if (!_is_analog[i]) {
            pin_cfg |= (1 << i);
            if (_is_digital_output[i]) {
                gpio_cfg |= (1 << i);
                gpo_drive |= (1 << i); // push-pull
            }
        }
    }

    write_register(REG_PIN_CFG, pin_cfg);
    write_register(REG_GPIO_CFG, gpio_cfg);
    write_register(REG_GPO_DRIVE, gpo_drive);

    return true;
}

// ---------------- Pin config ----------------
void ADS7138::set_pin_analog(uint8_t channel) {
    if (channel > 7) return;
    _is_analog[channel] = true;
    _is_digital_output[channel] = false;
    _is_digital_input[channel] = false;
}

void ADS7138::set_pin_digital_output(uint8_t channel, bool initial_level) {
    if (channel > 7) return;
    _is_analog[channel] = false;
    _is_digital_output[channel] = true;
    _is_digital_input[channel] = false;
    set_digital(channel, initial_level);
}

void ADS7138::set_pin_digital_input(uint8_t channel) {
    if (channel > 7) return;
    _is_analog[channel] = false;
    _is_digital_output[channel] = false;
    _is_digital_input[channel] = true;
}

// ---------------- Oversampling ----------------
void ADS7138::set_oversampling(uint8_t channel, uint8_t samples) {
    if (channel > 7) return;
    _oversampling[channel] = samples ? samples : 1; // avoid 0 samples
}

// ---------------- Digital ----------------
void ADS7138::set_digital(uint8_t channel, bool level) {
    if (channel > 7 || !_is_digital_output[channel]) return;

    uint8_t gpo_value = read_register(REG_GPO_VALUE);
    if (level)
        gpo_value |= (1 << channel);
    else
        gpo_value &= ~(1 << channel);

    write_register(REG_GPO_VALUE, gpo_value);
}

bool ADS7138::read_digital(uint8_t channel) {
    if (channel > 7) return false;
    uint8_t val = read_register(REG_GPI_VALUE);
    return (val >> channel) & 0x01;
}

// ---------------- Analog ----------------
double ADS7138::read_analog(uint8_t channel) {
    if (channel > 7 || !_is_analog[channel]) return 0.0;

    uint32_t sum = 0;
    for (uint8_t i = 0; i < _oversampling[channel]; i++) {
        uint16_t adc_val = read_channel_register(channel);
        sum += adc_val;
    }

    double avg = (double)sum / _oversampling[channel];
    return avg * _v_ref / 4095.0;
}

// ---------------- I2C helpers ----------------
void ADS7138::write_register(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(_i2c_addr);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t ADS7138::read_register(uint8_t reg) {
    Wire.beginTransmission(_i2c_addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(_i2c_addr, (uint8_t)1);
    if (Wire.available()) return Wire.read();
    return 0;
}

uint16_t ADS7138::read_channel_register(uint8_t channel) {
    uint8_t msb = 0, lsb = 0;
    uint8_t reg = REG_CONVERSION + channel;

    Wire.beginTransmission(_i2c_addr);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(_i2c_addr, (uint8_t)2);
    if (Wire.available() >= 2) {
        msb = Wire.read();
        lsb = Wire.read();
    }

    return ((msb & 0x0F) << 8) | lsb;
}
