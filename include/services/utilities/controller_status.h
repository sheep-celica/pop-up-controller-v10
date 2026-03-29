#pragma once

#include <cstdint>

bool initialize_controller_bench_mode(float battery_voltage);
bool is_controller_bench_mode_enabled();
void update_bench_mode_led_indicator(uint32_t now_ms);
