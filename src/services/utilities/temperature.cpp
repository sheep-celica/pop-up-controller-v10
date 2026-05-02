#include "services/utilities/temperature.h"

#include <Arduino.h>
#include <cmath>
#include <cstdlib>
#include <Wire.h>

#include "config.h"
#include "services/logging/logging.h"

namespace {
    constexpr uint8_t kTmp112TemperatureRegister = 0x00;

    Generic_LM75 hotspot_lm75_sensor(config::hardware::temperature::HOTSPOT_SENSOR_ADDRESS);

    struct TemperatureSensorConfig
    {
        bool supported;
        uint8_t address;
        config::hardware::temperature::SensorDriver driver;
    };

    TemperatureSensorConfig get_sensor_config(TemperatureSensorRole role)
    {
        switch (role)
        {
            case TemperatureSensorRole::Ambient:
                return {
                    config::hardware::temperature::HAS_AMBIENT_SENSOR,
                    config::hardware::temperature::AMBIENT_SENSOR_ADDRESS,
                    config::hardware::temperature::AMBIENT_SENSOR_DRIVER,
                };

            case TemperatureSensorRole::Hotspot:
                return {
                    config::hardware::temperature::HAS_HOTSPOT_SENSOR,
                    config::hardware::temperature::HOTSPOT_SENSOR_ADDRESS,
                    config::hardware::temperature::HOTSPOT_SENSOR_DRIVER,
                };
        }

        return { false, 0x00, config::hardware::temperature::SensorDriver::None };
    }

    bool is_i2c_device_present(uint8_t address)
    {
        Wire.beginTransmission(address);
        return Wire.endTransmission(true) == 0;
    }

    bool read_tmp112_temperature(uint8_t address, float& celsius)
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
        const uint16_t raw_register = (static_cast<uint16_t>(msb) << 8) | lsb;

        int16_t raw_12_bit = static_cast<int16_t>(raw_register) >> 4;
        if ((raw_12_bit & 0x0800) != 0) {
            raw_12_bit |= 0xF000;
        }

        celsius = static_cast<float>(raw_12_bit) * 0.0625f;
        return true;
    }

    bool read_sensor_temperature(TemperatureSensorRole role, const TemperatureSensorConfig& sensor, float& celsius)
    {
        switch (sensor.driver)
        {
            case config::hardware::temperature::SensorDriver::Lm75Compatible:
                if (role != TemperatureSensorRole::Hotspot) {
                    return false;
                }

                celsius = hotspot_lm75_sensor.readTemperatureC();
                return true;

            case config::hardware::temperature::SensorDriver::Tmp112:
                return read_tmp112_temperature(sensor.address, celsius);

            case config::hardware::temperature::SensorDriver::None:
                break;
        }

        return false;
    }

    void log_temperature_read_result(TemperatureSensorRole role, const TemperatureReadResult& result)
    {
        const char* role_name = temperature_sensor_role_name(role);

        if (!result.supported)
        {
            LOG("%s temperature sensor: Not supported on this board.", role_name);
            return;
        }

        if (!result.connected)
        {
            LOG("%s temperature sensor: Not Connected", role_name);
            return;
        }

        if (!result.read_ok)
        {
            LOG("%s temperature sensor: Read Failed", role_name);
            return;
        }

        LOG("%s temperature: %.2f C", role_name, result.celsius);
    }

    void format_temperature_result(TemperatureReadResult result, char* buffer, size_t buffer_size)
    {
        if (!buffer || buffer_size == 0) {
            return;
        }

        if (!result.supported)
        {
            snprintf(buffer, buffer_size, "Not Supported");
            return;
        }

        if (!result.connected)
        {
            snprintf(buffer, buffer_size, "Not Connected");
            return;
        }

        if (!result.read_ok)
        {
            snprintf(buffer, buffer_size, "Read Failed");
            return;
        }

        const long centi = lroundf(result.celsius * 100.0f);
        const long abs_centi = labs(centi);
        snprintf(
            buffer,
            buffer_size,
            (centi < 0) ? "-%ld.%02ld" : "%ld.%02ld",
            abs_centi / 100,
            abs_centi % 100);
    }
}

void setup_temperature()
{
    bool malfunction_detected = false;

    constexpr TemperatureSensorRole roles[] = {
        TemperatureSensorRole::Hotspot,
        TemperatureSensorRole::Ambient,
    };

    for (TemperatureSensorRole role : roles)
    {
        const TemperatureReadResult result = read_temperature(role);
        if (!result.supported) {
            continue;
        }

        log_temperature_read_result(role, result);
        if (!result.connected || !result.read_ok) {
            malfunction_detected = true;
        }
    }

    if (malfunction_detected) {
        report_error_code(ErrorCode::TEMP_SENSOR_MALFUNCTION);
    }
}

bool is_temperature_sensor_supported(TemperatureSensorRole role)
{
    return get_sensor_config(role).supported;
}

bool is_temperature_sensor_connected(TemperatureSensorRole role)
{
    const TemperatureSensorConfig sensor = get_sensor_config(role);
    if (!sensor.supported) {
        return false;
    }

    return is_i2c_device_present(sensor.address);
}

TemperatureReadResult read_temperature(TemperatureSensorRole role)
{
    TemperatureReadResult result;
    const TemperatureSensorConfig sensor = get_sensor_config(role);
    result.supported = sensor.supported;

    if (!result.supported) {
        return result;
    }

    result.connected = is_i2c_device_present(sensor.address);
    if (!result.connected) {
        return result;
    }

    result.read_ok = read_sensor_temperature(role, sensor, result.celsius);
    return result;
}

const char* temperature_sensor_role_name(TemperatureSensorRole role)
{
    switch (role)
    {
        case TemperatureSensorRole::Ambient:
            return "Ambient";

        case TemperatureSensorRole::Hotspot:
            return "Hotspot";
    }

    return "Unknown";
}

bool is_temperature_sensor_connected()
{
    return is_temperature_sensor_connected(TemperatureSensorRole::Hotspot);
}

float read_temperature()
{
    const TemperatureReadResult result = read_temperature(TemperatureSensorRole::Hotspot);
    return result.read_ok ? result.celsius : NAN;
}

const char* read_temperature_char()
{
    static char buf[16];
    format_temperature_result(read_temperature(TemperatureSensorRole::Hotspot), buf, sizeof(buf));
    return buf;
}
