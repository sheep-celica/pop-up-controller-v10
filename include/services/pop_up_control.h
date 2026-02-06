#pragma once
#include "helpers/pop_up.h"
#include "helpers/motor_controller.h"


// Declared class variables
extern PopUp RH_POP_UP;
extern PopUp LH_POP_UP;
extern MotorController RH_MOTOR;
extern MotorController LH_MOTOR;


// Public functions
void setup_pop_ups();
void update_pop_ups();