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

    void log_print_pop_up_min_state_persist_ms_usage()
    {
        LOG("Usage: printPopUpMinStatePersistMs");
    }

    void handle_print_pop_up_min_state_persist_ms_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* extra_token = next_token(cursor);
        if (extra_token)
        {
            LOG("printPopUpMinStatePersistMs rejected: this command does not take arguments.");
            log_print_pop_up_min_state_persist_ms_usage();
            return;
        }

        LOG(
            "MIN_STATE_PERSIST_MS=%lu",
            static_cast<unsigned long>(get_pop_up_min_state_persist_ms()));
    }
}

extern const CommandDefinition kPrintPopUpMinStatePersistMsCommandDefinition = {
    "printPopUpMinStatePersistMs",
    "printPopUpMinStatePersistMs",
    handle_print_pop_up_min_state_persist_ms_command
};
