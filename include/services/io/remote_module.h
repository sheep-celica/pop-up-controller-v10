#pragma once

#include <cstdint>

void setup_remote_module();
void update_remote_module();
bool is_remote_module_supported();
bool is_remote_module_io_connected();
bool is_remote_module_radio_connected();
bool is_remote_module_connected();
uint8_t get_remote_module_radio_part_number();
uint8_t get_remote_module_radio_version();
bool read_remote_module_pair_button();
bool set_remote_module_pair_led(bool on);
bool set_remote_module_status_led(bool on);
bool set_remote_module_debug_led(bool on);
void log_remote_module_status();
bool start_remote_module_pairing();
bool is_remote_module_pairing();
bool has_paired_remote();
uint32_t get_paired_remote_id();
bool test_remote_module_communication_timing(uint16_t samples);
bool test_remote_wake_signal(uint8_t duration_seconds);
bool test_remote_carrier_sense(uint8_t duration_seconds);
bool test_remote_wor(uint8_t duration_seconds);
