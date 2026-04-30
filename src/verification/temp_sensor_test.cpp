#include "verification/temp_sensor_test.h"

#include <Arduino.h>
#include <Wire.h>

#include "services/logging/logging.h"

namespace {
    constexpr uint8_t kTmp112TemperatureRegister = 0x00;
    // TMP112 address mapping: ADD0 to GND = 0x48, ADD0 to VCC = 0x49.
    constexpr uint8_t kAmbientTempSensorAddress = 0x48;
    constexpr uint8_t kHotspotTempSensorAddress = 0x49;

    struct TempSensorCheck {
        const char* name;
        uint8_t address;
    };

    bool is_i2c_device_present(uint8_t address)
    {
        Wire.beginTransmission(address);
        return Wire.endTransmission() == 0;
    }

    bool read_tmp112_temperature(uint8_t address, uint16_t& raw_register, float& celsius)
    {
        Wire.beginTransmission(address);
        Wire.write(kTmp112TemperatureRegister);
        if (Wire.endTransmission(false) != 0) {
            return false;
        }

        const uint8_t bytes_read = Wire.requestFrom(address, static_cast<uint8_t>(2));
        if (bytes_read != 2 || Wire.available() < 2) {
            return false;
        }

        const uint8_t msb = Wire.read();
        const uint8_t lsb = Wire.read();
        raw_register = (static_cast<uint16_t>(msb) << 8) | lsb;

        int16_t raw_12_bit = static_cast<int16_t>(raw_register) >> 4;
        if ((raw_12_bit & 0x0800) != 0) {
            raw_12_bit |= 0xF000;
        }

        celsius = static_cast<float>(raw_12_bit) * 0.0625f;
        return true;
    }

    void check_temp_sensor(const TempSensorCheck& sensor)
    {
        const bool present = is_i2c_device_present(sensor.address);
        LOG(
            "%s TMP112 at 0x%02X: %s.",
            sensor.name,
            sensor.address,
            present ? "present" : "not present");

        if (!present) {
            return;
        }

        uint16_t raw_register = 0;
        float celsius = 0.0f;
        if (!read_tmp112_temperature(sensor.address, raw_register, celsius)) {
            LOG("%s TMP112 temperature read failed.", sensor.name);
            return;
        }

        LOG(
            "%s TMP112 temperature: raw=0x%04X, %.2f C.",
            sensor.name,
            raw_register,
            celsius);
    }
}

void setup_temp_sensor_test()
{
    constexpr TempSensorCheck sensors[] = {
        { "Ambient", kAmbientTempSensorAddress },
        { "Hotspot", kHotspotTempSensorAddress },
    };

    LOG("TMP112 temperature sensor verification started.");
    for (const TempSensorCheck& sensor : sensors) {
        check_temp_sensor(sensor);
    }
    LOG("TMP112 temperature sensor verification complete.");
}
