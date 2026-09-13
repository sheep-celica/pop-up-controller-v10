#pragma once
#include <cstdint>

bool diagnostics_active();
bool diagnostics_motion_active();
void diagnostics_enter();
void diagnostics_exit();
void diagnostics_select_rh();
void diagnostics_select_lh();
void diagnostics_select_bh();
void diagnostics_update();
void diagnostics_record_switch_command(bool rh, uint8_t from, uint8_t to);
void diagnostics_print();
bool diagnostics_clear();
bool diagnostics_light_switch_off();
