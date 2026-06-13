#pragma once

#include "board_config.h"

#if POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER
#include "helpers/DRV8243.h"

extern DRV8243 RH_DRV8243_MOTOR;
extern DRV8243 LH_DRV8243_MOTOR;
#else
#include "helpers/motor_controller.h"

extern MotorController RH_MOTOR;
extern MotorController LH_MOTOR;
#endif

bool setup_motors();
void update_motors();
void prepare_motors_for_sleep();
