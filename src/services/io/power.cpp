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
    uint32_t s_next_idle_countdown_log_s = 0;
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

    void schedule_next_idle_countdown_log(uint32_t elapsed_ms)
    {
        const uint32_t countdown_step_s = config::pins::power::IDLE_COUNTDOWN_LOG_STEP_S;
        if (countdown_step_s == 0u || s_idle_time_to_power_off_s <= countdown_step_s) {
            s_next_idle_countdown_log_s = 0;
            return;
        }

        const uint32_t elapsed_s = elapsed_ms / 1000u;
        if (elapsed_s >= s_idle_time_to_power_off_s) {
            s_next_idle_countdown_log_s = 0;
            return;
        }

        const uint32_t remaining_s = s_idle_time_to_power_off_s - elapsed_s;
        uint32_t next_log_s = (remaining_s / countdown_step_s) * countdown_step_s;

        // Keep the next milestone strictly below current remaining time so we don't log immediately.
        if (next_log_s >= remaining_s) {
            if (next_log_s > countdown_step_s) {
                next_log_s -= countdown_step_s;
            } else {
                next_log_s = 0;
            }
        }

        s_next_idle_countdown_log_s = next_log_s;
    }

    bool is_light_switch_active_raw()
    {
        // Light-switch lines are wired active-low: LOW means pressed/active.
        return digitalRead(config::pins::LIGHT_SWITCH_UP_PIN) == LOW ||
               digitalRead(config::pins::LIGHT_SWITCH_HOLD_PIN) == LOW;
    }

    void save_pop_up_timing_calibrations_before_power_off()
    {
        const bool rh_saved = RH_POP_UP.timing_calibration.save_to_preferences(RH_PREFS);
        const bool lh_saved = LH_POP_UP.timing_calibration.save_to_preferences(LH_PREFS);

        if (rh_saved && lh_saved) {
            LOG("Saved RH/LH pop-up timing calibrations to NVS.");
            return;
        }

        if (!rh_saved && !lh_saved) {
            LOG("Failed to save RH and LH pop-up timing calibrations to NVS.");
            return;
        }

        if (!rh_saved) {
            LOG("Failed to save RH pop-up timing calibration to NVS.");
        } else {
            LOG("Failed to save LH pop-up timing calibration to NVS.");
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
    schedule_next_idle_countdown_log(millis() - s_last_idle_reset_ms);
    return true;
}

void setup_power()
{
    pinMode(config::pins::POWER_ON_PIN, OUTPUT);
    ensure_power_preferences();
    reset_idle_time();

    constexpr uint32_t kSecondsPerMinute = 60u;
    constexpr uint32_t kSecondsPerHour = 60u * kSecondsPerMinute;
    constexpr uint32_t kSecondsPerDay = 24u * kSecondsPerHour;

    const uint32_t days = s_idle_time_to_power_off_s / kSecondsPerDay;
    const uint32_t after_days = s_idle_time_to_power_off_s % kSecondsPerDay;
    const uint32_t hours = after_days / kSecondsPerHour;
    const uint32_t after_hours = after_days % kSecondsPerHour;
    const uint32_t minutes = after_hours / kSecondsPerMinute;
    const uint32_t seconds = after_hours % kSecondsPerMinute;

    LOG(
        "Idle power-off threshold: %lu s (%lu d %lu h %lu m %lu s).",
        static_cast<unsigned long>(s_idle_time_to_power_off_s),
        static_cast<unsigned long>(days),
        static_cast<unsigned long>(hours),
        static_cast<unsigned long>(minutes),
        static_cast<unsigned long>(seconds));
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
    save_pop_up_timing_calibrations_before_power_off();
    s_power_off_requested = true;
    LOG("Latching power OFF.");
    Serial.flush();  // Ensure Serial log finishes before shutdown
    
    digitalWrite(config::pins::POWER_ON_PIN, LOW);
}

void reset_idle_time()
{
    s_last_idle_reset_ms = millis();
    s_power_off_requested = false;
    schedule_next_idle_countdown_log(0);
}

void check_idle_time()
{
    ensure_power_preferences();

    if (s_power_off_requested) {
        return;
    }

    // While either light-switch command line is held active, keep extending idle timeout.
    if (is_light_switch_active_raw()) {
        reset_idle_time();
        return;
    }

    const uint32_t elapsed_ms = millis() - s_last_idle_reset_ms;
    const uint32_t idle_timeout_ms = s_idle_time_to_power_off_s * 1000u;
    const uint32_t remaining_ms = (elapsed_ms < idle_timeout_ms) ? (idle_timeout_ms - elapsed_ms) : 0u;
    const uint32_t remaining_s = (remaining_ms + 999u) / 1000u; // ceil to whole seconds

    while (s_next_idle_countdown_log_s > 0u && remaining_s <= s_next_idle_countdown_log_s)
    {
        LOG(
            "Idle power-off in %lu s.",
            static_cast<unsigned long>(s_next_idle_countdown_log_s));

        const uint32_t countdown_step_s = config::pins::power::IDLE_COUNTDOWN_LOG_STEP_S;
        if (countdown_step_s == 0u) {
            s_next_idle_countdown_log_s = 0u;
        } else if (s_next_idle_countdown_log_s > countdown_step_s) {
            s_next_idle_countdown_log_s -= countdown_step_s;
        } else {
            s_next_idle_countdown_log_s = 0u;
        }
    }

    if (elapsed_ms >= idle_timeout_ms)
    {
        LOG(
            "Idle timeout reached after %lu s (threshold %lu s).",
            static_cast<unsigned long>(elapsed_ms / 1000u),
            static_cast<unsigned long>(s_idle_time_to_power_off_s));
        power_off();
    }
}
