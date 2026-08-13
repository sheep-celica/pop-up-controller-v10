#include "services/commands/command_definitions.h"

#include "services/io/motors.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"

namespace {
    void handle_print_motor_current_calibration_command(char* remaining_args)
    {
        (void)remaining_args;

        if (!are_pop_ups_idle_or_timed_out()) {
            LOG("printMotorCurrentCalibration rejected: pop-ups must be idle or timed out.");
            return;
        }

        (void)print_motor_current_calibration();
    }
}

extern const CommandDefinition kPrintMotorCurrentCalibrationCommandDefinition = {
    "printMotorCurrentCalibration",
    "printMotorCurrentCalibration",
    handle_print_motor_current_calibration_command
};
