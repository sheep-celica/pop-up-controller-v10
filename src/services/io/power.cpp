#include <Arduino.h>
#include <esp_sleep.h>

#include "services/io/power.h"
#include "services/io/leds.h"
#include "services/io/motors.h"
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
    bool s_power_latch_unsupported_logged = false;

    const char* idle_power_action_name_impl()
    {
        if (config::features::HAS_POWER_LATCH) {
            return "power-off";
        }

        if (config::features::HAS_DEEP_SLEEP_WAKE) {
            return "deep-sleep";
        }

        return "shutdown";
    }

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

    bool is_deep_sleep_wake_line_active_raw()
    {
        if (!config::features::HAS_DEEP_SLEEP_WAKE ||
            config::pins::power::DEEP_SLEEP_WAKE_PIN == GPIO_NUM_NC) {
            return false;
        }

        return digitalRead(config::pins::power::DEEP_SLEEP_WAKE_PIN) == LOW;
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

    void save_runtime_state_before_power_transition()
    {
        statistics_manager.flush_deferred_counters();
        save_pop_up_timing_calibrations_before_power_off();
    }

    void log_power_latch_unsupported_once()
    {
        if (s_power_latch_unsupported_logged) {
            return;
        }

        LOG("Power latch is not available on %s.", config::board::DISPLAY_NAME);
        s_power_latch_unsupported_logged = true;
    }

    void log_idle_threshold()
    {
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
            "Idle %s threshold: %lu s (%lu d %lu h %lu m %lu s).",
            idle_power_action_name_impl(),
            static_cast<unsigned long>(s_idle_time_to_power_off_s),
            static_cast<unsigned long>(days),
            static_cast<unsigned long>(hours),
            static_cast<unsigned long>(minutes),
            static_cast<unsigned long>(seconds));
    }

    void prepare_pop_up_sensing_for_sleep()
    {
        pinMode(config::pins::RH_SENSE_PIN, OUTPUT);
        pinMode(config::pins::LH_SENSE_PIN, OUTPUT);
        digitalWrite(config::pins::RH_SENSE_PIN, LOW);
        digitalWrite(config::pins::LH_SENSE_PIN, LOW);
    }

    bool is_deep_sleep_supported_impl()
    {
        return config::features::HAS_DEEP_SLEEP_WAKE &&
               config::pins::power::DEEP_SLEEP_WAKE_PIN != GPIO_NUM_NC;
    }

    bool enter_deep_sleep()
    {
        const gpio_num_t wake_pin = config::pins::power::DEEP_SLEEP_WAKE_PIN;
        if (wake_pin == GPIO_NUM_NC)
        {
            LOG("Deep-sleep entry aborted: no wake pin is configured for this board.");
            s_power_off_requested = false;
            return false;
        }

        LOG("Saving data before deep sleep.");
        save_runtime_state_before_power_transition();
        s_power_off_requested = true;

        prepare_leds_for_sleep();
        prepare_motors_for_sleep();
        prepare_pop_up_sensing_for_sleep();

        if (config::features::HAS_DRV8243_MOTOR_DRIVER)
        {
            digitalWrite(config::pins::motor_driver::SHARED_SLEEP_PIN, LOW);
            pinMode(config::pins::motor_driver::SHARED_SLEEP_PIN, OUTPUT);
        }

        pinMode(wake_pin, INPUT);

        LOG(
            "Entering deep sleep now. Wake source: GPIO %u LOW.",
            static_cast<unsigned>(wake_pin));
        Serial.flush();

        esp_sleep_enable_ext0_wakeup(wake_pin, 0);
        esp_deep_sleep_start();

        LOG("Deep-sleep entry returned unexpectedly.");
        s_power_off_requested = false;
        return false;
    }
}

bool is_valid_idle_time_to_power_off_seconds(uint32_t idle_time_to_power_off_s)
{
    return is_valid_idle_time_to_power_off_seconds_impl(idle_time_to_power_off_s);
}

const char* get_idle_power_action_name()
{
    return idle_power_action_name_impl();
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

bool reset_power_configuration_to_default()
{
    ensure_power_preferences();

    if (!s_power_preferences.clear())
    {
        return false;
    }

    s_idle_time_to_power_off_s = config::pins::power::IDLE_TIME_TO_POWER_OFF_S;
    s_idle_time_to_power_off_loaded = true;
    schedule_next_idle_countdown_log(millis() - s_last_idle_reset_ms);
    return true;
}

void setup_power()
{
    ensure_power_preferences();
    reset_idle_time();

    if (config::features::HAS_POWER_LATCH)
    {
        // Latch power immediately in case the physical light switch briefly opens.
        digitalWrite(config::pins::power::POWER_LATCH_PIN, HIGH);
        pinMode(config::pins::power::POWER_LATCH_PIN, OUTPUT);
    }

    if (config::features::HAS_DEEP_SLEEP_WAKE &&
        config::pins::power::DEEP_SLEEP_WAKE_PIN != GPIO_NUM_NC)
    {
        pinMode(config::pins::power::DEEP_SLEEP_WAKE_PIN, INPUT);
    }

    if (config::features::HAS_POWER_LATCH || config::features::HAS_DEEP_SLEEP_WAKE)
    {
        log_idle_threshold();
    }
}

void power_on()
{
    reset_idle_time();

    if (config::features::HAS_DEEP_SLEEP_WAKE && !config::features::HAS_POWER_LATCH) {
        return;
    }

    if (!config::features::HAS_POWER_LATCH) {
        log_power_latch_unsupported_once();
        return;
    }

    LOG("Latching power ON.");
    digitalWrite(config::pins::power::POWER_LATCH_PIN, HIGH);
}

void power_off()
{
    if (config::features::HAS_DEEP_SLEEP_WAKE && !config::features::HAS_POWER_LATCH)
    {
        enter_deep_sleep();
        return;
    }

    LOG("Saving data before power OFF.");
    save_runtime_state_before_power_transition();
    s_power_off_requested = true;

    if (!config::features::HAS_POWER_LATCH) {
        log_power_latch_unsupported_once();
        Serial.flush();
        return;
    }

    LOG("Latching power OFF.");
    Serial.flush();  // Ensure Serial log finishes before shutdown
    
    digitalWrite(config::pins::power::POWER_LATCH_PIN, LOW);
}

bool is_deep_sleep_supported()
{
    return is_deep_sleep_supported_impl();
}

bool force_deep_sleep()
{
    if (!is_deep_sleep_supported_impl())
    {
        LOG("Deep sleep is not supported on this board.");
        return false;
    }

    return enter_deep_sleep();
}

void reboot_controller()
{
    if (config::features::HAS_POWER_LATCH) {
        power_off();
        delay(100);
    } else {
        LOG("Saving data before reboot.");
        save_runtime_state_before_power_transition();
        Serial.flush();
    }

    ESP.restart();
}

void reset_idle_time()
{
    s_last_idle_reset_ms = millis();
    s_power_off_requested = false;
    schedule_next_idle_countdown_log(0);
}

void check_idle_time()
{
    if (!config::features::HAS_POWER_LATCH && !config::features::HAS_DEEP_SLEEP_WAKE) {
        return;
    }

    ensure_power_preferences();

    if (s_power_off_requested) {
        return;
    }

    if (config::features::HAS_POWER_LATCH)
    {
        // While either light-switch command line is held active, keep extending idle timeout.
        if (is_light_switch_active_raw()) {
            reset_idle_time();
            return;
        }
    }

    if (config::features::HAS_DEEP_SLEEP_WAKE)
    {
        if (is_deep_sleep_wake_line_active_raw() || !are_pop_ups_idle_or_timed_out()) {
            reset_idle_time();
            return;
        }
    }

    const uint32_t elapsed_ms = millis() - s_last_idle_reset_ms;
    const uint32_t idle_timeout_ms = s_idle_time_to_power_off_s * 1000u;
    const uint32_t remaining_ms = (elapsed_ms < idle_timeout_ms) ? (idle_timeout_ms - elapsed_ms) : 0u;
    const uint32_t remaining_s = (remaining_ms + 999u) / 1000u; // ceil to whole seconds

    while (s_next_idle_countdown_log_s > 0u && remaining_s <= s_next_idle_countdown_log_s)
    {
        LOG(
            "Idle %s in %lu s.",
            idle_power_action_name_impl(),
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
            "Idle timeout reached after %lu s (threshold %lu s). Entering %s.",
            static_cast<unsigned long>(elapsed_ms / 1000u),
            static_cast<unsigned long>(s_idle_time_to_power_off_s),
            idle_power_action_name_impl());
        power_off();
    }
}

bool is_idle_power_off_supported()
{
    return config::features::HAS_POWER_LATCH || config::features::HAS_DEEP_SLEEP_WAKE;
}
