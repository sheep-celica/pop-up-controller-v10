#include "services/commands/command_definitions.h"

#include "services/logging/logging.h"
#include "services/utilities/controller_status.h"

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

    void log_get_controller_status_usage()
    {
        LOG("Usage: getControllerStatus");
    }

    void handle_get_controller_status_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* extra_token = next_token(cursor);
        if (extra_token)
        {
            LOG("getControllerStatus rejected: this command does not take arguments.");
            log_get_controller_status_usage();
            return;
        }

        LOG("%s", is_controller_bench_mode_enabled() ? "BENCH MODE" : "RUNNING");
    }
}

extern const CommandDefinition kGetControllerStatusCommandDefinition = {
    "getControllerStatus",
    "getControllerStatus",
    handle_get_controller_status_command
};
