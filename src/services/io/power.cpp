#include "services/io/power.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "config.h"

#include <Preferences.h>

namespace {
    constexpr const char* kPowerConfigurationNamespace = "power_cfg";
    constexpr const char* kIdleTimeToPowerOffKey = "idle_s";
    constexpr uint32_t kMaxIdleTimeToPowerOffSeconds = UINT32_MAX / 1000u;

    Preferences s_power_preferences;
    bool s_power_preferences_initialized = false;
    bool s_idle_time_to_power_off_loaded = false;
    uint32_t s_idle_time_to_power_off_s = config::pins::power::IDLE_TIME_TO_POWER_OFF_S;
    uint32_t s_last_idle_reset_ms = 0;
    bool s_power_off_requested = false;

    bool is_valid_idle_time_to_power_off_seconds_impl(uint32_t idle_time_to_power_off_s)
    {
        return idle_time_to_power_off_s > 0u && idle_time_to_power_off_s <= kMaxIdleTimeToPowerOffSeconds;
    }

    void ensure_power_preferences()
    {
        if (!s_power_preferences_initialized)
        {
            s_power_preferences.begin(kPowerConfigurationNamespace, false);
            s_power_preferences_initialized = true;
        }

        if (!s_idle_time_to_power_off_loaded)
        {
            if (s_power_preferences.isKey(kIdleTimeToPowerOffKey)) {
                s_idle_time_to_power_off_s = s_power_preferences.getUInt(
                    kIdleTimeToPowerOffKey,
                    config::pins::power::IDLE_TIME_TO_POWER_OFF_S);
            } else {
                s_idle_time_to_power_off_s = config::pins::power::IDLE_TIME_TO_POWER_OFF_S;
            }

            if (!is_valid_idle_time_to_power_off_seconds_impl(s_idle_time_to_power_off_s)) {
                s_idle_time_to_power_off_s = config::pins::power::IDLE_TIME_TO_POWER_OFF_S;
            }

            s_idle_time_to_power_off_loaded = true;
        }
    }
}

bool is_valid_idle_time_to_power_off_seconds(uint32_t idle_time_to_power_off_s)
{
    return is_valid_idle_time_to_power_off_seconds_impl(idle_time_to_power_off_s);
}

uint32_t get_idle_time_to_power_off_seconds()
{
    ensure_power_preferences();
    return s_idle_time_to_power_off_s;
}

bool set_idle_time_to_power_off_seconds(uint32_t idle_time_to_power_off_s)
{
    if (!is_valid_idle_time_to_power_off_seconds_impl(idle_time_to_power_off_s)) {
        return false;
    }

    ensure_power_preferences();

    const size_t bytes_written = s_power_preferences.putUInt(
        kIdleTimeToPowerOffKey,
        idle_time_to_power_off_s);

    if (bytes_written != sizeof(uint32_t)) {
        return false;
    }

    s_idle_time_to_power_off_s = idle_time_to_power_off_s;
    s_idle_time_to_power_off_loaded = true;
    return true;
}

void setup_power()
{
    pinMode(config::pins::POWER_ON_PIN, OUTPUT);
    ensure_power_preferences();
    reset_idle_time();
    LOG("Idle power-off threshold: %lu s", static_cast<unsigned long>(s_idle_time_to_power_off_s));
}

void power_on()
{
    reset_idle_time();
    LOG("Latching power ON.");
    digitalWrite(config::pins::POWER_ON_PIN, HIGH);
}

void power_off()
{
    LOG("Saving data before power OFF.");
    RH_POP_UP.timing_calibration.save_to_preferences(RH_PREFS);
    LH_POP_UP.timing_calibration.save_to_preferences(LH_PREFS);
    s_power_off_requested = true;
    LOG("Latching power OFF.");
    Serial.flush();  // Ensure Serial log finishes before shutdown
    
    digitalWrite(config::pins::POWER_ON_PIN, LOW);
}

void reset_idle_time()
{
    s_last_idle_reset_ms = millis();
    s_power_off_requested = false;
}

void check_idle_time()
{
    ensure_power_preferences();

    if (s_power_off_requested) {
        return;
    }

    const uint32_t elapsed_ms = millis() - s_last_idle_reset_ms;
    const uint32_t idle_timeout_ms = s_idle_time_to_power_off_s * 1000u;

    if (elapsed_ms > idle_timeout_ms)
    {
        LOG(
            "Idle timeout reached after %lu s (threshold %lu s).",
            static_cast<unsigned long>(elapsed_ms / 1000u),
            static_cast<unsigned long>(s_idle_time_to_power_off_s));
        power_off();
    }
}
