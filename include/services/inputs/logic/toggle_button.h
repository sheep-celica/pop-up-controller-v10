#pragma once
#include "services/inputs/types/input.h"

// Call this once from setup() to register this button input + its logic task
void toggle_button_register();

extern Input toggle_button;
