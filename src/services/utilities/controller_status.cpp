#include "services/utilities/controller_status.h"

#include <Preferences.h>

#include "config.h"
#include "services/io/leds.h"

namespace {
    Preferences s_bench_mode_preferences;
    bool s_bench_mode_preferences_initialized = false;
    bool s_controller_bench_mode_enabled = false;
    bool s_automatic_bench_mode_active = false;
    bool s_forced_bench_mode_active = false;
    bool s_forced_bench_mode_configured = false;
    bool s_bench_led_on = false;
    uint32_t s_bench_led_last_toggle_ms = 0;
    constexpr uint32_t kBenchLedTogglePeriodMs = 100;

    void ensure_bench_mode_preferences()
    {
        if (!s_bench_mode_preferences_initialized)
        {
            s_bench_mode_preferences.begin(
                config::utilities::BENCH_MODE_NAMESPACE,
                false);
            s_bench_mode_preferences_initialized = true;
        }
    }
}

bool initialize_controller_bench_mode(float battery_voltage)
{
    ensure_bench_mode_preferences();
    s_forced_bench_mode_configured = s_bench_mode_preferences.getBool(
        config::utilities::FORCED_BENCH_MODE_KEY,
        false);
    s_forced_bench_mode_active = s_forced_bench_mode_configured;
    s_automatic_bench_mode_active =
        battery_voltage < config::utilities::BENCH_MODE_MAX_BATTERY_V;
    s_controller_bench_mode_enabled =
        s_automatic_bench_mode_active || s_forced_bench_mode_active;
    s_bench_led_last_toggle_ms = millis();
    s_bench_led_on = false;
    return s_controller_bench_mode_enabled;
}

bool is_automatic_bench_mode_active()
{
    return s_automatic_bench_mode_active;
}

bool is_forced_bench_mode_active()
{
    return s_forced_bench_mode_active;
}

bool is_forced_bench_mode_configured()
{
    return s_forced_bench_mode_configured;
}

bool save_forced_bench_mode_configuration(bool enabled)
{
    ensure_bench_mode_preferences();
    const size_t bytes_written = s_bench_mode_preferences.putBool(
        config::utilities::FORCED_BENCH_MODE_KEY,
        enabled);
    if (bytes_written != sizeof(bool))
    {
        return false;
    }

    // The active mode is intentionally fixed for the lifetime of this boot.
    // This prevents disabling the lock from immediately enabling live inputs.
    s_forced_bench_mode_configured = enabled;
    return true;
}

bool is_controller_bench_mode_enabled()
{
    return s_controller_bench_mode_enabled;
}

void update_bench_mode_led_indicator(uint32_t now_ms)
{
    if (!s_controller_bench_mode_enabled)
    {
        return;
    }

    if ((now_ms - s_bench_led_last_toggle_ms) < kBenchLedTogglePeriodMs)
    {
        return;
    }

    s_bench_led_last_toggle_ms = now_ms;
    s_bench_led_on = !s_bench_led_on;
    set_led_state(LedId::STATUS_LED, s_bench_led_on);
    set_led_state(LedId::INPUT_LED, s_bench_led_on);
}
