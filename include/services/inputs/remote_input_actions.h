#pragma once

#include <cstdint>

// Runs the action assigned to a logical remote input (1..4).
void trigger_remote_input_action(uint8_t logical_input_1_to_4);

// Consumes any paired CC1101/EV1527 button frame received this loop.
void update_remote_module_inputs();
