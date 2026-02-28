#pragma once

#include <cstdint>
#include <Preferences.h>

extern Preferences calibration_preferences;

float read_battery_voltage();
uint32_t measure_battery_voltage_read_duration_us(uint8_t sample_count, uint8_t repetitions = 1);
bool get_battery_voltage_calibration(float& a, float& b);
bool write_battery_voltage_calibration(float a, float b);
