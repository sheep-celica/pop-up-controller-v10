#include "services/commands/command_definitions.h"
#include "services/io/motors.h"
#include "services/pop_up_control/pop_up_control.h"
#include "services/io/leds.h"
#include "services/logging/logging.h"

namespace {
    void handle_clear_errors_command(char* remaining_args)
    {
        (void)remaining_args;
        if (!error_log_manager.clear_error_log_entries())
        {
            LOG("clearErrors failed: could not clear stored error log entries.");
            return;
        }

        LOG("clearErrors succeeded: all error log entries cleared.");
        RH_POP_UP.reset_timeout();
        LH_POP_UP.reset_timeout();
        clear_motor_stall_faults();
        set_led_state(LedId::ERROR_LED, false);
    }
}

extern const CommandDefinition kClearErrorsCommandDefinition = {
    "clearErrors",
    "clearErrors",
    handle_clear_errors_command
};
