#pragma once

#include <cstdint>

bool initialize_controller_bench_mode(float battery_voltage);
bool is_controller_bench_mode_enabled();
bool is_automatic_bench_mode_active();
bool is_forced_bench_mode_active();
bool is_forced_bench_mode_configured();
bool save_forced_bench_mode_configuration(bool enabled);
void update_bench_mode_led_indicator(uint32_t now_ms);
