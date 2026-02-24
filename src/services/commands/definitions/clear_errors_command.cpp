#include "services/commands/command_definitions.h"

#include "services/logging/logging.h"

namespace {
    void handle_clear_errors_command(char* remaining_args)
    {
        (void)remaining_args;
        error_log_manager.clear_error_log_entries();
        LOG("clearErrors succeeded: all error log entries cleared.");
    }
}

extern const CommandDefinition kClearErrorsCommandDefinition = {
    "clearErrors",
    "clearErrors",
    handle_clear_errors_command
};
