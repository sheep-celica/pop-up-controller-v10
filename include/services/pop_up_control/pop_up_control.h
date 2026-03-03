#pragma once
#include "helpers/pop_up.h"
#include "helpers/motor_controller.h"


// Declared class variables
extern PopUp RH_POP_UP;
extern PopUp LH_POP_UP;
extern Preferences RH_PREFS;
extern Preferences LH_PREFS;
extern MotorController RH_MOTOR;
extern MotorController LH_MOTOR;


// Public functions
void setup_pop_ups();
void update_pop_ups();
void safe_move_pop_up_to(PopUp*, PopUpState);
