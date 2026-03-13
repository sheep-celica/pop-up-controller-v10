#include "services/commands/command_definitions.h"

#include "services/io/io_expanders.h"
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

    void log_get_external_expander_usage()
    {
        LOG("Usage: getExternalExpander");
    }

    void handle_get_external_expander_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* extra_token = next_token(cursor);
        if (extra_token)
        {
            LOG("getExternalExpander rejected: this command does not take arguments.");
            log_get_external_expander_usage();
            return;
        }

        if (!is_external_expander_connected())
        {
            LOG("Not Connected");
            return;
        }

        LOG("0x%02X", static_cast<unsigned>(get_external_expander_i2c_address()));
    }
}

extern const CommandDefinition kGetExternalExpanderCommandDefinition = {
    "getExternalExpander",
    "getExternalExpander",
    handle_get_external_expander_command
};
