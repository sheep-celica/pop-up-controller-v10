#pragma once
#include "services/inputs/types/input.h"

// Call this once from setup() to register this button input + its logic task
void light_switch_up_register();

extern Input light_switch_up;