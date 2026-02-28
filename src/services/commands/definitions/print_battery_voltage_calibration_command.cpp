#include "services/commands/command_definitions.h"

#include "services/logging/logging.h"
#include "services/utilities/utilities.h"

namespace {
    void handle_print_battery_voltage_calibration_command(char* remaining_args)
    {
        (void)remaining_args;

        float a = 1.0f;
        float b = 0.0f;
        const bool has_stored_calibration = get_battery_voltage_calibration(a, b);

        if (has_stored_calibration) {
            LOG("Battery voltage calibration constants: a=%.6f, b=%.6f", a, b);
            return;
        }

        LOG("Battery voltage calibration constants not fully stored. Effective values: a=%.6f, b=%.6f", a, b);
    }
}

extern const CommandDefinition kPrintBatteryVoltageCalibrationCommandDefinition = {
    "printBatteryVoltageCalibration",
    "printBatteryVoltageCalibration",
    handle_print_battery_voltage_calibration_command
};
