#pragma once

#include <cstdint>

#include <Temperature_LM75_Derived.h>

enum class TemperatureSensorRole : uint8_t
{
    Ambient,
    Hotspot,
};

struct TemperatureReadResult
{
    bool supported = false;
    bool connected = false;
    bool read_ok = false;
    float celsius = 0.0f;
};

void setup_temperature();
bool is_temperature_sensor_supported(TemperatureSensorRole role);
bool is_temperature_sensor_connected(TemperatureSensorRole role);
TemperatureReadResult read_temperature(TemperatureSensorRole role);
const char* temperature_sensor_role_name(TemperatureSensorRole role);

// Compatibility helpers. The historical single temperature sensor is the hotspot sensor.
bool is_temperature_sensor_connected();
float read_temperature();
const char* read_temperature_char();
