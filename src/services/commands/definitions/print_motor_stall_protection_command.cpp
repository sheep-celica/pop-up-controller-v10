#include "services/commands/command_definitions.h"

#include "services/io/motors.h"
#include "services/logging/logging.h"

namespace {
    bool has_argument(const char* text)
    {
        if (!text) return false;
        while (*text == ' ' || *text == '\t') ++text;
        return *text != '\0';
    }

    void handle_print_motor_stall_protection_command(char* remaining_args)
    {
        if (has_argument(remaining_args)) {
            LOG("printMotorStallProtection rejected: this command does not take arguments.");
            LOG("Usage: printMotorStallProtection");
            return;
        }

        (void)print_motor_stall_protection_config();
    }
}

extern const CommandDefinition kPrintMotorStallProtectionCommandDefinition = {
    "printMotorStallProtection",
    "printMotorStallProtection",
    handle_print_motor_stall_protection_command
};
