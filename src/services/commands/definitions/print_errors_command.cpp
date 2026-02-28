#include "services/commands/command_definitions.h"

#include "services/logging/logging.h"

namespace {
    void handle_print_errors_command(char* remaining_args)
    {
        (void)remaining_args;
        LOG("printErrors command received.");
        error_log_manager.print_error_log_entries();
    }
}

extern const CommandDefinition kPrintErrorsCommandDefinition = {
    "printErrors",
    "printErrors",
    handle_print_errors_command
};
