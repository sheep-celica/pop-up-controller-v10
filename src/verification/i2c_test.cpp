#include "verification/i2c_test.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "services/logging/logging.h"

void setup_i2c_verification_bus()
{
    Wire.begin(config::pins::i2c::SDA, config::pins::i2c::SCL);
    Wire.setTimeOut(config::pins::i2c::TIMEOUT_MS);
    Wire.setClock(config::pins::i2c::FREQUENCY_HZ);
}

void print_i2c_scan()
{
    uint8_t found_count = 0;

    LOG("I2C scan started.");
    for (uint8_t address = 1; address < 0x7F; ++address) {
        Wire.beginTransmission(address);
        const uint8_t error = Wire.endTransmission();
        if (error == 0) {
            LOG("I2C device found at 0x%02X.", address);
            ++found_count;
        }
    }

    LOG("I2C scan complete. Devices found: %u.", found_count);
}
