#include "services/commands/command_definitions.h"

#include "services/io/remote_module.h"
#include "services/logging/logging.h"

namespace {
    void handle_get_remote_module_command(char* remaining_args)
    {
        if (remaining_args && remaining_args[0] != '\0') {
            LOG("getRemoteModule rejected: this command does not take arguments.");
            LOG("Usage: getRemoteModule");
            return;
        }
        log_remote_module_status();
    }
}

extern const CommandDefinition kGetRemoteModuleCommandDefinition = {
    "getRemoteModule",
    "getRemoteModule",
    handle_get_remote_module_command
};
