#include "services/commands/command_definitions.h"

#include "services/inputs/remote_input_pins.h"
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

    void log_print_remote_inputs_with_headlights_usage()
    {
        LOG("Usage: printRemoteInputsWithHeadlights");
    }

    void handle_print_remote_inputs_with_headlights_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* extra_token = next_token(cursor);
        if (extra_token)
        {
            LOG("printRemoteInputsWithHeadlights rejected: this command does not take arguments.");
            log_print_remote_inputs_with_headlights_usage();
            return;
        }

        LOG(
            "ALLOW_REMOTE_INPUTS_WITH_HEADLIGHTS=%s",
            are_remote_inputs_with_headlights_allowed() ? "TRUE" : "FALSE");
    }
}

extern const CommandDefinition kPrintRemoteInputsWithHeadlightsCommandDefinition = {
    "printRemoteInputsWithHeadlights",
    "printRemoteInputsWithHeadlights",
    handle_print_remote_inputs_with_headlights_command
};
