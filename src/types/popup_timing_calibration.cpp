#include "types/pop_up_timing_calibration.h"
#include <string.h>


PopUpTimingCalibration::PopUpTimingCalibration()
{
    // 0 indicates "no data"
    memset(down_time_table, 0, sizeof(down_time_table));
}

uint16_t PopUpTimingCalibration::voltage_to_decivolt(float voltage)
{
    return static_cast<uint16_t>(voltage * 10.0f + 0.5f);
}

uint16_t PopUpTimingCalibration::clamp_voltage_decivolt(uint16_t dv)
{
    if (dv < MIN_VOLTAGE_DV) return MIN_VOLTAGE_DV;
    if (dv > MAX_VOLTAGE_DV) return MAX_VOLTAGE_DV;
    return dv;
}

bool PopUpTimingCalibration::add_sample(float battery_voltage, uint32_t down_time_ms)
{
    uint16_t deci_volt = voltage_to_decivolt(battery_voltage);

    if (deci_volt < MIN_VOLTAGE_DV || deci_volt > MAX_VOLTAGE_DV)
    {
        return false;
    }

    size_t index = deci_volt - MIN_VOLTAGE_DV;
    down_time_table[index] = static_cast<uint16_t>(down_time_ms);

    return true;
}

uint32_t PopUpTimingCalibration::get_expected_down_time(float battery_voltage) const
{
    uint16_t deci_volt = voltage_to_decivolt(battery_voltage);
    deci_volt = clamp_voltage_decivolt(deci_volt);

    size_t index = deci_volt - MIN_VOLTAGE_DV;

    // Exact value available
    if (down_time_table[index] != 0)
    {
        return down_time_table[index];
    }

    // Find nearest lower and upper known samples
    int lower = static_cast<int>(index) - 1;
    int upper = static_cast<int>(index) + 1;

    while (lower >= 0 || upper < static_cast<int>(VOLTAGE_TABLE_SIZE))
    {
        if (lower >= 0 && down_time_table[lower] != 0 &&
            upper < static_cast<int>(VOLTAGE_TABLE_SIZE) &&
            down_time_table[upper] != 0)
        {
            // Linear interpolation
            float dv_lower = float(lower + MIN_VOLTAGE_DV);
            float dv_upper = float(upper + MIN_VOLTAGE_DV);

            float ratio = float(deci_volt - dv_lower) / float(dv_upper - dv_lower);

            return static_cast<uint32_t>(
                down_time_table[lower] + ratio * (down_time_table[upper] - down_time_table[lower])
            );
        }

        if (lower >= 0 && down_time_table[lower] != 0)
        {
            return down_time_table[lower];
        }

        if (upper < static_cast<int>(VOLTAGE_TABLE_SIZE) &&
            down_time_table[upper] != 0)
        {
            return down_time_table[upper];
        }

        --lower;
        ++upper;
    }

    // No calibration data at all
    return 0;
}

bool PopUpTimingCalibration::save_to_preferences(Preferences& prefs) const
{
    size_t written = prefs.putBytes(
        PREFERENCES_KEY,
        down_time_table,
        sizeof(down_time_table)
    );

    return written == sizeof(down_time_table);
}

bool PopUpTimingCalibration::load_from_preferences(Preferences& prefs)
{
    size_t read = prefs.getBytes(
        PREFERENCES_KEY,
        down_time_table,
        sizeof(down_time_table)
    );

    return read == sizeof(down_time_table);
}
