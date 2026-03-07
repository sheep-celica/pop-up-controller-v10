#include "services/commands/command_definitions.h"

#include <cstdlib>

#include "services/io/power.h"
#include "services/logging/logging.h"

namespace {
    void log_idle_time_to_power_off_value(uint32_t total_seconds)
    {
        constexpr uint32_t kSecondsPerMinute = 60u;
        constexpr uint32_t kSecondsPerHour = 60u * kSecondsPerMinute;
        constexpr uint32_t kSecondsPerDay = 24u * kSecondsPerHour;

        const uint32_t days = total_seconds / kSecondsPerDay;
        const uint32_t after_days = total_seconds % kSecondsPerDay;
        const uint32_t hours = after_days / kSecondsPerHour;
        const uint32_t after_hours = after_days % kSecondsPerHour;
        const uint32_t minutes = after_hours / kSecondsPerMinute;
        const uint32_t seconds = after_hours % kSecondsPerMinute;

        LOG(
            "Idle power-off threshold updated to %lu s (%lu d %lu h %lu m %lu s).",
            static_cast<unsigned long>(total_seconds),
            static_cast<unsigned long>(days),
            static_cast<unsigned long>(hours),
            static_cast<unsigned long>(minutes),
            static_cast<unsigned long>(seconds));
    }

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

        log_idle_time_to_power_off_value(get_idle_time_to_power_off_seconds());
    }
}

extern const CommandDefinition kWriteIdleTimeToPowerOffSecondsCommandDefinition = {
    "writeIdleTimeToPowerOffSeconds",
    "writeIdleTimeToPowerOffSeconds <seconds>",
    handle_write_idle_time_to_power_off_seconds_command
};
