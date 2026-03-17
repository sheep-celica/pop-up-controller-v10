#include "services/commands/command_definitions.h"

#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"

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

    void log_print_pop_up_sensing_delay_us_usage()
    {
        LOG("Usage: printPopUpSensingDelayUs");
    }

    void handle_print_pop_up_sensing_delay_us_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* extra_token = next_token(cursor);
        if (extra_token)
        {
            LOG("printPopUpSensingDelayUs rejected: this command does not take arguments.");
            log_print_pop_up_sensing_delay_us_usage();
            return;
        }

        LOG(
            "SENSING_DELAY_US=%lu",
            static_cast<unsigned long>(get_pop_up_sensing_delay_us()));
    }
}

extern const CommandDefinition kPrintPopUpSensingDelayUsCommandDefinition = {
    "printPopUpSensingDelayUs",
    "printPopUpSensingDelayUs",
    handle_print_pop_up_sensing_delay_us_command
};
