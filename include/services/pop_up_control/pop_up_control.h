#pragma once
#include <Preferences.h>
#include "helpers/pop_up.h"


// Declared class variables
extern PopUp RH_POP_UP;
extern PopUp LH_POP_UP;
extern Preferences RH_PREFS;
extern Preferences LH_PREFS;


// Public functions
void setup_pop_ups();
void update_pop_ups();
bool are_pop_ups_idle_or_timed_out();
void latch_pop_up_motion_disable(PopUpId pop_up_id, const char* reason);
void safe_move_pop_up_to(PopUp*, PopUpState);
bool toggle_sleepy_eye_mode();
bool save_sleepy_eye_mode_state();
void restore_sleepy_eye_mode_indicator();
bool is_sleepy_eye_mode_with_headlights_allowed();
bool set_sleepy_eye_mode_with_headlights_allowed(bool allowed);
bool reset_pop_up_configuration_to_defaults();
uint32_t get_pop_up_min_state_persist_ms();
bool set_pop_up_min_state_persist_ms(uint32_t min_state_persist_ms);
uint32_t get_pop_up_sensing_delay_us();
bool set_pop_up_sensing_delay_us(uint32_t sensing_delay_us);
