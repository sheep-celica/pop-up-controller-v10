#include "services/utilities/controller_status.h"

#include "config.h"
#include "services/io/leds.h"

namespace {
    bool s_controller_bench_mode_enabled = false;
    bool s_bench_led_on = false;
    uint32_t s_bench_led_last_toggle_ms = 0;
    constexpr uint32_t kBenchLedTogglePeriodMs = 100;
}

bool initialize_controller_bench_mode(float battery_voltage)
{
    s_controller_bench_mode_enabled =
        battery_voltage < config::utilities::BENCH_MODE_MAX_BATTERY_V;
    s_bench_led_last_toggle_ms = millis();
    s_bench_led_on = false;
    return s_controller_bench_mode_enabled;
}

bool is_controller_bench_mode_enabled()
{
    return s_controller_bench_mode_enabled;
}

void set_controller_bench_mode_enabled(bool enabled)
{
    s_controller_bench_mode_enabled = enabled;
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
