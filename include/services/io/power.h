#pragma once
#include <cstdint>


void setup_power();
void power_on();
void power_off();
void reset_idle_time();
void check_idle_time();
bool is_valid_idle_time_to_power_off_seconds(uint32_t idle_time_to_power_off_s);
uint32_t get_idle_time_to_power_off_seconds();
bool set_idle_time_to_power_off_seconds(uint32_t idle_time_to_power_off_s);
