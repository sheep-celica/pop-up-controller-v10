#include "services/commands/command_definitions.h"

#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"

namespace {
    void handle_print_sleepy_eye_mode_with_headlights_command(char* remaining_args)
    {
        (void)remaining_args;
        LOG(
            "ALLOW_SLEEPY_EYE_MODE_WITH_HEADLIGHTS=%s",
            is_sleepy_eye_mode_with_headlights_allowed() ? "TRUE" : "FALSE");
    }
}

extern const CommandDefinition kPrintSleepyEyeModeWithHeadlightsCommandDefinition = {
    "printSleepyEyeModeWithHeadlights",
    "printSleepyEyeModeWithHeadlights",
    handle_print_sleepy_eye_mode_with_headlights_command
};
