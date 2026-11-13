#include "services/commands/command_definitions.h"

#include "services/io/remote_module.h"
#include "services/logging/logging.h"

namespace {
    void pair_remote_command_handler(char* args)
    {
        if (args && args[0] != '\0') {
            LOG("pairRemote rejected: this command does not take arguments.");
            LOG("Usage: pairRemote");
            return;
        }

        if (!start_remote_module_pairing()) {
            LOG("Remote pairing could not start; the external remote module is not connected.");
        }
    }
}

extern const CommandDefinition kPairRemoteCommandDefinition = {
    "pairRemote",
    "pairRemote",
    pair_remote_command_handler,
    true
};
