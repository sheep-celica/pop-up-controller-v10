#pragma once
#include "services/inputs/types/input.h"

// Call this once from setup() to register this button input + its logic task
void light_switch_up_register();
bool is_light_switch_safely_off();

extern Input light_switch_up;