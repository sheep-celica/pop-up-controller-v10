#pragma once

#include "board_config.h"

enum class MotorCalibrationScope : uint8_t
{
    RH,
    LH,
    BOTH
};

struct MotorStallProtectionConfig
{
    bool enabled;
    float current_a;
    uint32_t duration_ms;
    uint32_t startup_blanking_ms;
};

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
void clear_motor_stall_faults();
bool get_motor_stall_protection_config(MotorStallProtectionConfig& stall_config);
bool save_motor_stall_protection_config(const MotorStallProtectionConfig& stall_config);
bool print_motor_stall_protection_config();
bool calibrate_motor_current(MotorCalibrationScope scope, uint32_t duration_ms);
bool save_motor_current_calibration(
    MotorCalibrationScope scope,
    float scale,
    float offset_a);
bool print_motor_current_calibration();
bool test_motor_current(uint32_t duration_ms);
