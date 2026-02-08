#pragma once
#include "services/inputs/types/input.h"

// Call this once from setup() to register this button input + its logic task
void lh_button_register();

extern Input lh_button;