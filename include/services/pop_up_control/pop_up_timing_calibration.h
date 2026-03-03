#ifndef POPUP_TIMING_CALIBRATION_H
#define POPUP_TIMING_CALIBRATION_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

/**
 * Stores and interpolates pop-up DOWN movement time as a function of
 * battery voltage.
 *
 * Voltage buckets are quantized to 0.1 V steps across the configured
 * range in config.h. Each bucket stores a running-average time and
 * the number of samples that contributed to that average.
 */
class PopUpTimingCalibration
{
public:
    PopUpTimingCalibration();

    // Clear the in-memory table.
    void clear();

    // Add one measured DOWN move time into the matching 0.1 V bucket.
    // Returns false if the voltage is out of range or the timing is invalid.
    bool add_sample(float battery_voltage, uint32_t down_time_ms);

    // True if any bucket contains at least one sample.
    bool has_samples() const;

    // True if the exact quantized voltage bucket contains data.
    bool has_exact_sample(float battery_voltage) const;

    // Returns the exact stored bucket average, or 0 if none exists.
    uint32_t get_exact_down_time(float battery_voltage) const;

    // Returns how many samples contributed to the exact bucket.
    uint16_t get_sample_count(float battery_voltage) const;

    // Returns the exact bucket if present, otherwise interpolates between
    // the nearest lower and upper populated buckets. Returns the configured
    // default time if no calibration data exists yet.
    uint32_t get_expected_down_time(float battery_voltage) const;

    // Save/load the table using the configured short Preferences key.
    // Use a separate Preferences namespace for each pop-up instance.
    bool save_to_preferences(Preferences& prefs) const;
    bool load_from_preferences(Preferences& prefs);

private:
    static constexpr uint16_t MIN_VOLTAGE_DV = config::pop_up::timing_calibration::MIN_BATTERY_VOLTAGE_DV;
    static constexpr uint16_t MAX_VOLTAGE_DV = config::pop_up::timing_calibration::MAX_BATTERY_VOLTAGE_DV;
    static constexpr uint16_t MIN_DOWN_TIME_MS = config::pop_up::timing_calibration::MIN_DOWN_TIME_MS;
    static constexpr uint16_t MAX_DOWN_TIME_MS = config::pop_up::timing_calibration::MAX_DOWN_TIME_MS;
    static constexpr size_t VOLTAGE_TABLE_SIZE = MAX_VOLTAGE_DV - MIN_VOLTAGE_DV + 1;

    struct PersistedData
    {
        uint16_t average_down_time_ms[VOLTAGE_TABLE_SIZE];
        uint16_t sample_count[VOLTAGE_TABLE_SIZE];
    };

    PersistedData data_;

    static uint16_t voltage_to_decivolt(float voltage);
    static bool is_supported_decivolt(uint16_t deci_volt);
    static bool is_supported_down_time_ms(uint32_t down_time_ms);
    static uint16_t clamp_voltage_decivolt(uint16_t deci_volt);
    static size_t decivolt_to_index(uint16_t deci_volt);
};

#endif // POPUP_TIMING_CALIBRATION_H
