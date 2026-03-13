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

    void log_get_idle_time_to_power_off_usage()
    {
        LOG("Usage: getIdleTimeToPowerOff");
    }

    void handle_get_idle_time_to_power_off_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* extra_token = next_token(cursor);
        if (extra_token)
        {
            LOG("getIdleTimeToPowerOff rejected: this command does not take arguments.");
            log_get_idle_time_to_power_off_usage();
            return;
        }

        LOG("%lu", static_cast<unsigned long>(get_idle_time_to_power_off_seconds()));
    }
}

extern const CommandDefinition kGetIdleTimeToPowerOffCommandDefinition = {
    "getIdleTimeToPowerOff",
    "getIdleTimeToPowerOff",
    handle_get_idle_time_to_power_off_command
};
