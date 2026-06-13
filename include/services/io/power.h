#pragma once
#include <cstdint>


void setup_power();
void power_on();
void power_off();
bool is_deep_sleep_supported();
bool force_deep_sleep();
void reboot_controller();
void reset_idle_time();
void check_idle_time();
bool is_idle_power_off_supported();
const char* get_idle_power_action_name();
bool is_valid_idle_time_to_power_off_seconds(uint32_t idle_time_to_power_off_s);
uint32_t get_idle_time_to_power_off_seconds();
bool set_idle_time_to_power_off_seconds(uint32_t idle_time_to_power_off_s);
