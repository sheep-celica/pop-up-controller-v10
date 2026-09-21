#include "services/commands/command_definitions.h"

#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"

namespace {
    void handle_print_sleepy_eye_position_override_command(char* remaining_args)
    {
        if (remaining_args)
        {
            while (*remaining_args == ' ' || *remaining_args == '\t') ++remaining_args;
            if (*remaining_args != '\0')
            {
                LOG("printSleepyEyePositionOverride rejected: this command does not take arguments.");
                LOG("Usage: printSleepyEyePositionOverride");
                return;
            }
        }
        LOG(
            "SLEEPY_EYE_POSITION_OVERRIDE enabled=%s position=%u",
            is_sleepy_eye_position_override_enabled() ? "TRUE" : "FALSE",
            static_cast<unsigned>(get_sleepy_eye_position_override()));
    }
}

extern const CommandDefinition kPrintSleepyEyePositionOverrideCommandDefinition = {
    "printSleepyEyePositionOverride",
    "printSleepyEyePositionOverride",
    handle_print_sleepy_eye_position_override_command
};
