#include "services/pop_up_control/pop_up_timing_calibration.h"

#include <cmath>
#include <cstring>

namespace {
    constexpr uint16_t kMaxStoredSampleCount = 0xFFFFu;
}

PopUpTimingCalibration::PopUpTimingCalibration()
{
    clear();
}

void PopUpTimingCalibration::clear()
{
    memset(&data_, 0, sizeof(data_));
}

uint16_t PopUpTimingCalibration::voltage_to_decivolt(float voltage)
{
    if (voltage <= 0.0f) {
        return 0;
    }

    return static_cast<uint16_t>((voltage * 10.0f) + 0.5f);
}

bool PopUpTimingCalibration::is_supported_decivolt(uint16_t deci_volt)
{
    return deci_volt >= MIN_VOLTAGE_DV && deci_volt <= MAX_VOLTAGE_DV;
}

bool PopUpTimingCalibration::is_supported_down_time_ms(uint32_t down_time_ms)
{
    return down_time_ms >= MIN_DOWN_TIME_MS && down_time_ms <= MAX_DOWN_TIME_MS;
}

uint16_t PopUpTimingCalibration::clamp_voltage_decivolt(uint16_t deci_volt)
{
    if (deci_volt < MIN_VOLTAGE_DV) return MIN_VOLTAGE_DV;
    if (deci_volt > MAX_VOLTAGE_DV) return MAX_VOLTAGE_DV;
    return deci_volt;
}

size_t PopUpTimingCalibration::decivolt_to_index(uint16_t deci_volt)
{
    return static_cast<size_t>(deci_volt - MIN_VOLTAGE_DV);
}

bool PopUpTimingCalibration::add_sample(float battery_voltage, uint32_t down_time_ms)
{
    if (!is_supported_down_time_ms(down_time_ms)) {
        return false;
    }

    const uint16_t deci_volt = voltage_to_decivolt(battery_voltage);
    if (!is_supported_decivolt(deci_volt)) {
        return false;
    }

    const size_t index = decivolt_to_index(deci_volt);
    const uint16_t stored_down_time_ms = static_cast<uint16_t>(down_time_ms);

    uint16_t& average_down_time_ms = data_.average_down_time_ms[index];
    uint16_t& sample_count = data_.sample_count[index];

    if (sample_count == 0)
    {
        average_down_time_ms = stored_down_time_ms;
        sample_count = 1;
        return true;
    }

    if (sample_count < kMaxStoredSampleCount)
    {
        const uint16_t new_sample_count = static_cast<uint16_t>(sample_count + 1u);
        const uint32_t weighted_sum =
            (static_cast<uint32_t>(average_down_time_ms) * sample_count) + stored_down_time_ms;

        average_down_time_ms = static_cast<uint16_t>((weighted_sum + (new_sample_count / 2u)) / new_sample_count);
        sample_count = new_sample_count;
        return true;
    }

    const uint32_t weighted_sum =
        (static_cast<uint32_t>(average_down_time_ms) * (sample_count - 1u)) + stored_down_time_ms;
    average_down_time_ms = static_cast<uint16_t>((weighted_sum + (sample_count / 2u)) / sample_count);
    return true;
}

bool PopUpTimingCalibration::has_samples() const
{
    for (size_t i = 0; i < VOLTAGE_TABLE_SIZE; ++i)
    {
        if (data_.sample_count[i] != 0) {
            return true;
        }
    }

    return false;
}

bool PopUpTimingCalibration::has_exact_sample(float battery_voltage) const
{
    const uint16_t deci_volt = voltage_to_decivolt(battery_voltage);
    if (!is_supported_decivolt(deci_volt)) {
        return false;
    }

    return data_.sample_count[decivolt_to_index(deci_volt)] != 0;
}

uint32_t PopUpTimingCalibration::get_exact_down_time(float battery_voltage) const
{
    const uint16_t deci_volt = voltage_to_decivolt(battery_voltage);
    if (!is_supported_decivolt(deci_volt)) {
        return 0;
    }

    const size_t index = decivolt_to_index(deci_volt);
    if (data_.sample_count[index] == 0) {
        return 0;
    }

    return data_.average_down_time_ms[index];
}

uint16_t PopUpTimingCalibration::get_sample_count(float battery_voltage) const
{
    const uint16_t deci_volt = voltage_to_decivolt(battery_voltage);
    if (!is_supported_decivolt(deci_volt)) {
        return 0;
    }

    return data_.sample_count[decivolt_to_index(deci_volt)];
}

uint32_t PopUpTimingCalibration::get_expected_down_time(float battery_voltage) const
{
    if (!has_samples()) {
        return config::pop_up::timing_calibration::DEFAULT_DOWN_TIME_MS;
    }

    uint16_t deci_volt = voltage_to_decivolt(battery_voltage);
    deci_volt = clamp_voltage_decivolt(deci_volt);

    const size_t index = decivolt_to_index(deci_volt);
    if (data_.sample_count[index] != 0) {
        return data_.average_down_time_ms[index];
    }

    int lower_index = -1;
    int upper_index = -1;

    for (int i = static_cast<int>(index) - 1; i >= 0; --i)
    {
        if (data_.sample_count[i] != 0)
        {
            lower_index = i;
            break;
        }
    }

    for (size_t i = index + 1; i < VOLTAGE_TABLE_SIZE; ++i)
    {
        if (data_.sample_count[i] != 0)
        {
            upper_index = static_cast<int>(i);
            break;
        }
    }

    if (lower_index < 0) {
        return data_.average_down_time_ms[upper_index];
    }

    if (upper_index < 0) {
        return data_.average_down_time_ms[lower_index];
    }

    const float lower_voltage_dv = static_cast<float>(lower_index + MIN_VOLTAGE_DV);
    const float upper_voltage_dv = static_cast<float>(upper_index + MIN_VOLTAGE_DV);
    const float lower_time_ms = static_cast<float>(data_.average_down_time_ms[lower_index]);
    const float upper_time_ms = static_cast<float>(data_.average_down_time_ms[upper_index]);

    const float ratio =
        (static_cast<float>(deci_volt) - lower_voltage_dv) / (upper_voltage_dv - lower_voltage_dv);

    return static_cast<uint32_t>(lroundf(lower_time_ms + (ratio * (upper_time_ms - lower_time_ms))));
}

bool PopUpTimingCalibration::save_to_preferences(Preferences& prefs) const
{
    const size_t written = prefs.putBytes(
        config::pop_up::timing_calibration::PREFERENCES_KEY,
        &data_,
        sizeof(data_));

    return written == sizeof(data_);
}

bool PopUpTimingCalibration::load_from_preferences(Preferences& prefs)
{
    if (!prefs.isKey(config::pop_up::timing_calibration::PREFERENCES_KEY))
    {
        clear();
        return false;
    }

    PersistedData loaded_data{};
    const size_t read = prefs.getBytes(
        config::pop_up::timing_calibration::PREFERENCES_KEY,
        &loaded_data,
        sizeof(loaded_data));

    if (read != sizeof(loaded_data))
    {
        clear();
        return false;
    }

    for (size_t i = 0; i < VOLTAGE_TABLE_SIZE; ++i)
    {
        if (loaded_data.sample_count[i] == 0)
        {
            loaded_data.average_down_time_ms[i] = 0;
        }
        else if (loaded_data.average_down_time_ms[i] == 0)
        {
            loaded_data.sample_count[i] = 0;
        }
    }

    data_ = loaded_data;
    return true;
}
