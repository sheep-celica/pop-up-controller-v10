#include "services/commands/command_definitions.h"

#include <cerrno>
#include <cstdlib>

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

    bool try_parse_min_state_persist_ms(char* token, uint32_t& min_state_persist_ms)
    {
        if (!token || token[0] == '\0') {
            return false;
        }

        if (token[0] == '-') {
            return false;
        }

        char* end = nullptr;
        errno = 0;
        const unsigned long parsed = strtoul(token, &end, 10);
        if (errno == ERANGE || end == token || !end || *end != '\0') {
            return false;
        }

        min_state_persist_ms = static_cast<uint32_t>(parsed);
        return true;
    }

    void log_write_pop_up_min_state_persist_ms_usage()
    {
        LOG("Usage: writePopUpMinStatePersistMs <milliseconds>");
    }

    void handle_write_pop_up_min_state_persist_ms_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* ms_token = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (!ms_token || extra_token)
        {
            LOG("writePopUpMinStatePersistMs rejected: expected exactly one integer argument.");
            log_write_pop_up_min_state_persist_ms_usage();
            return;
        }

        uint32_t min_state_persist_ms = 0;
        if (!try_parse_min_state_persist_ms(ms_token, min_state_persist_ms))
        {
            LOG("writePopUpMinStatePersistMs rejected: invalid integer argument.");
            log_write_pop_up_min_state_persist_ms_usage();
            return;
        }

        if (!set_pop_up_min_state_persist_ms(min_state_persist_ms))
        {
            LOG("writePopUpMinStatePersistMs failed: could not apply value.");
            return;
        }

        LOG(
            "MIN_STATE_PERSIST_MS set to %lu",
            static_cast<unsigned long>(get_pop_up_min_state_persist_ms()));
    }
}

extern const CommandDefinition kWritePopUpMinStatePersistMsCommandDefinition = {
    "writePopUpMinStatePersistMs",
    "writePopUpMinStatePersistMs <milliseconds>",
    handle_write_pop_up_min_state_persist_ms_command
};
