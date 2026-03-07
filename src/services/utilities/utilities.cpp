#include "services/utilities/utilities.h"
#include "services/io/io_expanders.h"
#include "config.h"
#include <Arduino.h>

// Calibration Preferences (extern declared in header)
Preferences calibration_preferences;
static bool calibration_preferences_initialized = false;
static void ensure_calibration_preferences();

namespace {
	constexpr float kDefaultBatteryVoltageCalibrationA = 1.0f;
	constexpr float kDefaultBatteryVoltageCalibrationB = 0.0f;
	bool s_battery_voltage_calibration_cached = false;
	bool s_battery_voltage_calibration_complete = false;
	float s_battery_voltage_calibration_a = kDefaultBatteryVoltageCalibrationA;
	float s_battery_voltage_calibration_b = kDefaultBatteryVoltageCalibrationB;

	uint8_t sanitize_sample_count(uint8_t sample_count)
	{
		return sample_count == 0 ? 1 : sample_count;
	}

	float read_calibration_float_if_present(const char* key, float default_value)
	{
		if (!calibration_preferences.isKey(key)) {
			return default_value;
		}

		return calibration_preferences.getFloat(key, default_value);
	}

	void ensure_battery_voltage_calibration_cache()
	{
		if (s_battery_voltage_calibration_cached) {
			return;
		}

		const bool has_a = calibration_preferences.isKey(config::utilities::calibration_keys::BAT_VOLTAGE_CONSTANT_A);
		const bool has_b = calibration_preferences.isKey(config::utilities::calibration_keys::BAT_VOLTAGE_CONSTANT_B);

		s_battery_voltage_calibration_a = read_calibration_float_if_present(
			config::utilities::calibration_keys::BAT_VOLTAGE_CONSTANT_A,
			kDefaultBatteryVoltageCalibrationA);
		s_battery_voltage_calibration_b = read_calibration_float_if_present(
			config::utilities::calibration_keys::BAT_VOLTAGE_CONSTANT_B,
			kDefaultBatteryVoltageCalibrationB);
		s_battery_voltage_calibration_complete = has_a && has_b;
		s_battery_voltage_calibration_cached = true;
	}

	float read_battery_pin_voltage_average(uint8_t sample_count)
	{
		const uint8_t effective_sample_count = sanitize_sample_count(sample_count);
		const uint8_t ch = static_cast<uint8_t>(config::pins::internal_expander::BATTERY_VOLTAGE_PIN);

		float voltage_sum = 0.0f;
		for (uint8_t i = 0; i < effective_sample_count; ++i)
		{
			voltage_sum += internal_ads.readAnalogVolts(ch, i == 0);
		}

		return voltage_sum / static_cast<float>(effective_sample_count);
	}

	float apply_battery_voltage_calibration(float measured_voltage)
	{
		ensure_calibration_preferences();
		return (s_battery_voltage_calibration_a * measured_voltage) + s_battery_voltage_calibration_b;
	}

	float read_battery_voltage_with_sample_count(uint8_t sample_count)
	{
		const float pin_voltage = read_battery_pin_voltage_average(sample_count);
		const float battery_voltage = pin_voltage * config::utilities::BATTERY_DIVIDER_SCALE;
		return apply_battery_voltage_calibration(battery_voltage);
	}
}

static void ensure_calibration_preferences()
{
	if (!calibration_preferences_initialized)
	{
		calibration_preferences.begin(config::utilities::CALIBRATION_NAMESPACE, false);
		calibration_preferences_initialized = true;
	}

	ensure_battery_voltage_calibration_cache();
}

bool get_battery_voltage_calibration(float& a, float& b)
{
	ensure_calibration_preferences();

	a = s_battery_voltage_calibration_a;
	b = s_battery_voltage_calibration_b;
	return s_battery_voltage_calibration_complete;
}

bool write_battery_voltage_calibration(float a, float b)
{
	ensure_calibration_preferences();

	const size_t bytes_written_a = calibration_preferences.putFloat(
		config::utilities::calibration_keys::BAT_VOLTAGE_CONSTANT_A,
		a);
	const size_t bytes_written_b = calibration_preferences.putFloat(
		config::utilities::calibration_keys::BAT_VOLTAGE_CONSTANT_B,
		b);

	if (bytes_written_a == sizeof(float) && bytes_written_b == sizeof(float))
	{
		s_battery_voltage_calibration_a = a;
		s_battery_voltage_calibration_b = b;
		s_battery_voltage_calibration_complete = true;
		s_battery_voltage_calibration_cached = true;
		return true;
	}

	s_battery_voltage_calibration_cached = false;
	return false;
}

uint32_t measure_battery_voltage_read_duration_us(uint8_t sample_count, uint8_t repetitions)
{
	const uint8_t effective_repetitions = sanitize_sample_count(repetitions);
	const uint32_t start_us = micros();

	for (uint8_t i = 0; i < effective_repetitions; ++i)
	{
		(void)read_battery_voltage_with_sample_count(sample_count);
	}

	const uint32_t elapsed_us = micros() - start_us;
	return elapsed_us / effective_repetitions;
}


float read_battery_voltage()
{
	return read_battery_voltage_with_sample_count(config::utilities::BATTERY_VOLTAGE_AVERAGE_SAMPLES);
}
