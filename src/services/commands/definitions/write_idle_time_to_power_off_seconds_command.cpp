#include "services/commands/command_definitions.h"

#include <cstdlib>

#include "services/io/power.h"
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

    bool try_parse_idle_time_to_power_off_seconds(char* token, uint32_t& idle_time_to_power_off_s)
    {
        if (!token || token[0] == '\0') {
            return false;
        }

        char* end = nullptr;
        const unsigned long parsed = strtoul(token, &end, 10);
        if (end == token || !end || *end != '\0') {
            return false;
        }

        idle_time_to_power_off_s = static_cast<uint32_t>(parsed);
        return true;
    }

    void log_write_idle_time_to_power_off_seconds_usage()
    {
        LOG("Usage: writeIdleTimeToPowerOffSeconds <seconds>");
    }

    void handle_write_idle_time_to_power_off_seconds_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* seconds_token = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (!seconds_token || extra_token) {
            LOG("writeIdleTimeToPowerOffSeconds rejected: expected exactly one integer argument.");
            log_write_idle_time_to_power_off_seconds_usage();
            return;
        }

        uint32_t idle_time_to_power_off_s = 0;
        if (!try_parse_idle_time_to_power_off_seconds(seconds_token, idle_time_to_power_off_s)) {
            LOG("writeIdleTimeToPowerOffSeconds rejected: invalid integer argument.");
            log_write_idle_time_to_power_off_seconds_usage();
            return;
        }

        if (!is_valid_idle_time_to_power_off_seconds(idle_time_to_power_off_s)) {
            LOG("writeIdleTimeToPowerOffSeconds rejected: value must be a positive whole number of seconds within the supported range.");
            return;
        }

        if (!set_idle_time_to_power_off_seconds(idle_time_to_power_off_s)) {
            LOG("writeIdleTimeToPowerOffSeconds failed: could not persist the idle power-off threshold.");
            return;
        }

        LOG("Idle power-off threshold updated to %lu s.", static_cast<unsigned long>(get_idle_time_to_power_off_seconds()));
    }
}

extern const CommandDefinition kWriteIdleTimeToPowerOffSecondsCommandDefinition = {
    "writeIdleTimeToPowerOffSeconds",
    "writeIdleTimeToPowerOffSeconds <seconds>",
    handle_write_idle_time_to_power_off_seconds_command
};
