#include "services/utilities/utilities.h"
#include "services/io/io_expanders.h"
#include "config.h"
#include <Arduino.h>

// Calibration Preferences (extern declared in header)
Preferences calibration_preferences;
static bool calibration_preferences_initialized = false;

static void ensure_calibration_preferences()
{
	if (!calibration_preferences_initialized)
	{
		calibration_preferences.begin(config::utilities::CALIBRATION_NAMESPACE, false);
		calibration_preferences_initialized = true;
	}
}


float read_battery_voltage()
{
	// Read the analog voltage from the internal ADS7138 on the configured pin
	uint8_t ch = static_cast<uint8_t>(config::pins::internal_expander::BATTERY_VOLTAGE_PIN);
	float pin_voltage = internal_ads.readAnalogVolts(ch);

	// Read divider scale from config (placeholder value if not present)
	float divider_scale = config::utilities::BATTERY_DIVIDER_SCALE;

	// Compute battery voltage from measured pin voltage
	float battery_voltage = pin_voltage * divider_scale;

	// Read calibration constants from preferences (provide sensible defaults)
	ensure_calibration_preferences();
	float calib_a = calibration_preferences.getFloat(config::utilities::calibration_keys::BAT_VOLTAGE_CONSTANT_A, 1.0f);
	float calib_b = calibration_preferences.getFloat(config::utilities::calibration_keys::BAT_VOLTAGE_CONSTANT_B, 0.0f);

	// Apply linear calibration: V_calibrated = a * V_measured + b
	float calibrated = calib_a * battery_voltage + calib_b;

	return calibrated;
}