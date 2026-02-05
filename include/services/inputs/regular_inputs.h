#pragma once

#include "services/inputs/input.h"


// Public access to inputs
extern Input light_switch_head;
extern Input light_switch_hold;
extern Input rh_button;
extern Input lh_button;
extern Input bh_button;
extern Input sleepy_eye_button;
extern Input debug_button;

// register inputs
void register_regular_inputs();
void handle_regular_inputs();