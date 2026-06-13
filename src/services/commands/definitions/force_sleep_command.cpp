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

    void log_force_sleep_usage()
    {
        LOG("Usage: forceSleep");
    }

    void handle_force_sleep_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* extra_token = next_token(cursor);
        if (extra_token)
        {
            LOG("forceSleep rejected: this command does not take arguments.");
            log_force_sleep_usage();
            return;
        }

        if (!is_deep_sleep_supported())
        {
            LOG("forceSleep rejected: deep sleep is not supported on this board.");
            return;
        }

        LOG("forceSleep command received. Entering deep sleep now.");
        force_deep_sleep();
    }
}

extern const CommandDefinition kForceSleepCommandDefinition = {
    "forceSleep",
    "forceSleep",
    handle_force_sleep_command
};
