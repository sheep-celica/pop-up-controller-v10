#include "services/commands/command_definitions.h"

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

    void log_print_build_info_usage()
    {
        LOG("Usage: printBuildInfo");
    }

    void handle_print_build_info_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* extra_token = next_token(cursor);
        if (extra_token)
        {
            LOG("printBuildInfo rejected: this command does not take arguments.");
            log_print_build_info_usage();
            return;
        }

        const char* build_version = get_current_build_version();
        const char* build_timestamp = get_current_build_timestamp();

        LOG("FW_VERSION=%s", (build_version && build_version[0] != '\0') ? build_version : "<unknown>");
        LOG("BUILD_TIMESTAMP=%s", (build_timestamp && build_timestamp[0] != '\0') ? build_timestamp : "<unknown>");
    }
}

extern const CommandDefinition kPrintBuildInfoCommandDefinition = {
    "printBuildInfo",
    "printBuildInfo",
    handle_print_build_info_command
};
