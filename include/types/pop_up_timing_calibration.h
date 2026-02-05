#ifndef POPUP_TIMING_CALIBRATION_H
#define POPUP_TIMING_CALIBRATION_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * @brief Stores and interpolates pop-up DOWN movement timing
 *        as a function of battery voltage.
 *
 * Voltage range:
 *   10.0 V – 20.0 V (inclusive)
 * Resolution:
 *   0.1 V
 *
 * Internally, voltage is quantized to decivolts (e.g. 12.3 V → 123).
 */
class PopUpTimingCalibration
{
public:
    /**
     * @brief Preferences key used for persistent storage
     */
    static constexpr const char* PREFERENCES_KEY = "popup_down_timing";

    /**
     * @brief Construct a new calibration table
     *
     * All entries are initialized as empty.
     */
    PopUpTimingCalibration();

    /**
     * @brief Add or update a timing sample
     *
     * The voltage is rounded to 0.1 V resolution.
     * Samples outside the 10.0–20.0 V range are rejected.
     *
     * @param battery_voltage  Measured battery voltage (V)
     * @param down_time_ms     Measured DOWN movement time (ms)
     *
     * @return true  Sample accepted and stored
     * @return false Voltage out of supported range
     */
    bool add_sample(float battery_voltage, uint32_t down_time_ms);

    /**
     * @brief Get the expected DOWN movement time
     *
     * If an exact voltage entry exists, it is returned.
     * Otherwise, linear interpolation is performed between
     * the nearest lower and upper stored voltages.
     *
     * Voltages outside the supported range are clamped
     * to 10.0 V or 20.0 V.
     *
     * @param battery_voltage Current battery voltage (V)
     *
     * @return Expected DOWN movement time (ms),
     *         or 0 if no calibration data exists
     */
    uint32_t get_expected_down_time(float battery_voltage) const;

    /**
     * @brief Save calibration table to flash using Preferences
     *
     * @param prefs Open Preferences instance
     *
     * @return true  Save successful
     * @return false Save failed
     */
    bool save_to_preferences(Preferences& prefs) const;

    /**
     * @brief Load calibration table from flash using Preferences
     *
     * @param prefs Open Preferences instance
     *
     * @return true  Load successful
     * @return false Load failed or data missing
     */
    bool load_from_preferences(Preferences& prefs);

private:
    static constexpr uint16_t MIN_VOLTAGE_DV = 100; // 10.0 V
    static constexpr uint16_t MAX_VOLTAGE_DV = 200; // 20.0 V
    static constexpr size_t VOLTAGE_TABLE_SIZE = MAX_VOLTAGE_DV - MIN_VOLTAGE_DV + 1;

    // Index 0 = 10.0 V, index 100 = 20.0 V
    uint16_t down_time_table[VOLTAGE_TABLE_SIZE];

    static uint16_t voltage_to_decivolt(float voltage);
    static uint16_t clamp_voltage_decivolt(uint16_t dv);
};

#endif // POPUP_TIMING_CALIBRATION_H
