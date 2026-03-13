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

    void log_print_remote_input_pins_usage()
    {
        LOG("Usage: printRemoteInputPins");
    }

    void handle_print_remote_input_pins_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* extra_token = next_token(cursor);
        if (extra_token)
        {
            LOG("printRemoteInputPins rejected: this command does not take arguments.");
            log_print_remote_input_pins_usage();
            return;
        }

        const RemoteInputPinMapping mapping = get_remote_input_pin_mapping();
        LOG(
            "REMOTE_INPUT_PINS=%u %u %u %u",
            static_cast<unsigned>(mapping.remote_input_1),
            static_cast<unsigned>(mapping.remote_input_2),
            static_cast<unsigned>(mapping.remote_input_3),
            static_cast<unsigned>(mapping.remote_input_4));
    }
}

extern const CommandDefinition kPrintRemoteInputPinsCommandDefinition = {
    "printRemoteInputPins",
    "printRemoteInputPins",
    handle_print_remote_input_pins_command
};
