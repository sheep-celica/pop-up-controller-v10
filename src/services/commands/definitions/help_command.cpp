#include "services/commands/command_definitions.h"

#include "services/commands/commands_registry.h"
#include "services/logging/logging.h"

namespace {
    void handle_help_command(char* remaining_args)
    {
        (void)remaining_args;

        size_t count = 0;
        const CommandDefinition* commands = get_command_registry(count);

        LOG("Available commands:");
        for (size_t i = 0; i < count; ++i) {
            LOG("  %s", commands[i].usage);
        }
        LOG("Note: clear commands are placeholders right now.");
    }
}

extern const CommandDefinition kHelpCommandDefinition = {
    "help",
    "help",
    handle_help_command
};

