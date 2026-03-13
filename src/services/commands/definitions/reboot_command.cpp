#include "services/commands/command_definitions.h"

#include "services/io/power.h"
#include "services/logging/logging.h"

namespace {
    bool is_space_char(char c)
    {
        return c == ' ' || c == '\t';
    }

    char* next_token(char*& cursor)
    {
        if (!cursor) return nullptr;

        while (*cursor != '\0' && is_space_char(*cursor)) {
            ++cursor;
        }

        if (*cursor == '\0') {
            return nullptr;
        }

        char* token = cursor;
        while (*cursor != '\0' && !is_space_char(*cursor)) {
            ++cursor;
        }

        if (*cursor != '\0') {
            *cursor = '\0';
            ++cursor;
        }

        return token;
    }

    void log_reboot_usage()
    {
        LOG("Usage: reboot");
    }

    void handle_reboot_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* extra_token = next_token(cursor);
        if (extra_token)
        {
            LOG("reboot rejected: this command does not take arguments.");
            log_reboot_usage();
            return;
        }

        LOG("Reboot command received. Powering off and restarting.");
        reboot_controller();
    }
}

extern const CommandDefinition kRebootCommandDefinition = {
    "reboot",
    "reboot",
    handle_reboot_command
};
