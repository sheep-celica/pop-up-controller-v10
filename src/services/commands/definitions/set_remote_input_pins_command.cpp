#include "services/commands/command_definitions.h"

#include <cstdlib>

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

    bool try_parse_pin_number(char* token, uint8_t& pin_number_1_to_4)
    {
        if (!token || token[0] == '\0') {
            return false;
        }

        char* end = nullptr;
        const unsigned long parsed = strtoul(token, &end, 10);
        if (end == token || !end || *end != '\0' || parsed < 1ul || parsed > 4ul) {
            return false;
        }

        pin_number_1_to_4 = static_cast<uint8_t>(parsed);
        return true;
    }

    void log_set_remote_input_pins_usage()
    {
        LOG("Usage: setRemoteInputPins <input1> <input2> <input3> <input4>");
        LOG("Values must be 1..4 and each value must appear exactly once.");
        LOG("Default mapping: setRemoteInputPins 1 2 3 4");
    }

    void handle_set_remote_input_pins_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* input_1_token = next_token(cursor);
        char* input_2_token = next_token(cursor);
        char* input_3_token = next_token(cursor);
        char* input_4_token = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (!input_1_token || !input_2_token || !input_3_token || !input_4_token || extra_token)
        {
            LOG("setRemoteInputPins rejected: expected exactly four arguments.");
            log_set_remote_input_pins_usage();
            return;
        }

        RemoteInputPinMapping requested_mapping {};
        if (!try_parse_pin_number(input_1_token, requested_mapping.remote_input_1) ||
            !try_parse_pin_number(input_2_token, requested_mapping.remote_input_2) ||
            !try_parse_pin_number(input_3_token, requested_mapping.remote_input_3) ||
            !try_parse_pin_number(input_4_token, requested_mapping.remote_input_4))
        {
            LOG("setRemoteInputPins rejected: arguments must be integers 1..4.");
            log_set_remote_input_pins_usage();
            return;
        }

        if (!is_valid_remote_input_pin_mapping(requested_mapping))
        {
            LOG("setRemoteInputPins rejected: values must be unique and in range 1..4.");
            log_set_remote_input_pins_usage();
            return;
        }

        if (!set_remote_input_pin_mapping(requested_mapping))
        {
            LOG("setRemoteInputPins failed: could not persist mapping to NVS.");
            return;
        }

        const RemoteInputPinMapping applied_mapping = get_remote_input_pin_mapping();
        LOG(
            "Remote input mapping updated and saved: %u %u %u %u.",
            static_cast<unsigned>(applied_mapping.remote_input_1),
            static_cast<unsigned>(applied_mapping.remote_input_2),
            static_cast<unsigned>(applied_mapping.remote_input_3),
            static_cast<unsigned>(applied_mapping.remote_input_4));
    }
}

extern const CommandDefinition kSetRemoteInputPinsCommandDefinition = {
    "setRemoteInputPins",
    "setRemoteInputPins <input1> <input2> <input3> <input4>",
    handle_set_remote_input_pins_command
};
