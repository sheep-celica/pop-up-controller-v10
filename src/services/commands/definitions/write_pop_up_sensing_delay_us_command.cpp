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

    bool try_parse_sensing_delay_us(char* token, uint32_t& sensing_delay_us)
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

        sensing_delay_us = static_cast<uint32_t>(parsed);
        return true;
    }

    void log_write_pop_up_sensing_delay_us_usage()
    {
        LOG("Usage: writePopUpSensingDelayUs <microseconds>");
    }

    void handle_write_pop_up_sensing_delay_us_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* sensing_delay_token = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (!sensing_delay_token || extra_token)
        {
            LOG("writePopUpSensingDelayUs rejected: expected exactly one integer argument.");
            log_write_pop_up_sensing_delay_us_usage();
            return;
        }

        uint32_t sensing_delay_us = 0;
        if (!try_parse_sensing_delay_us(sensing_delay_token, sensing_delay_us))
        {
            LOG("writePopUpSensingDelayUs rejected: invalid integer argument.");
            log_write_pop_up_sensing_delay_us_usage();
            return;
        }

        if (!set_pop_up_sensing_delay_us(sensing_delay_us))
        {
            LOG("writePopUpSensingDelayUs failed: could not apply value.");
            return;
        }

        LOG(
            "SENSING_DELAY_US set to %lu",
            static_cast<unsigned long>(get_pop_up_sensing_delay_us()));
    }
}

extern const CommandDefinition kWritePopUpSensingDelayUsCommandDefinition = {
    "writePopUpSensingDelayUs",
    "writePopUpSensingDelayUs <microseconds>",
    handle_write_pop_up_sensing_delay_us_command
};
