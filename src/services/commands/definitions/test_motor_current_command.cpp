#include "services/commands/command_definitions.h"

#include <cstdint>
#include <cstdlib>

#include "config.h"
#include "services/io/motors.h"
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

    void log_usage()
    {
        LOG("Usage: testMotorCurrent [duration_ms]");
    }

    void handle_test_motor_current_command(char* remaining_args)
    {
        if (!are_pop_ups_idle_or_timed_out()) {
            LOG("testMotorCurrent rejected: pop-ups must be idle or timed out.");
            return;
        }

        char* cursor = remaining_args;
        char* duration_token = next_token(cursor);
        char* extra_token = next_token(cursor);
        if (extra_token) {
            LOG("testMotorCurrent rejected: too many arguments.");
            log_usage();
            return;
        }

        uint32_t duration_ms = config::motors::drv8243::CURRENT_TEST_DURATION_MS;
        if (duration_token) {
            char* parse_end = nullptr;
            const unsigned long parsed_duration = strtoul(duration_token, &parse_end, 10);
            if (*duration_token == '\0' || !parse_end || *parse_end != '\0' ||
                parsed_duration == 0 ||
                parsed_duration > config::motors::drv8243::CURRENT_TEST_MAX_DURATION_MS) {
                LOG("testMotorCurrent rejected: duration must be 1-%lu ms.",
                    static_cast<unsigned long>(
                        config::motors::drv8243::CURRENT_TEST_MAX_DURATION_MS));
                log_usage();
                return;
            }
            duration_ms = static_cast<uint32_t>(parsed_duration);
        }

        LOG("testMotorCurrent: duration=%lu ms per motor.",
            static_cast<unsigned long>(duration_ms));
        (void)test_motor_current(duration_ms);
    }
}

extern const CommandDefinition kTestMotorCurrentCommandDefinition = {
    "testMotorCurrent",
    "testMotorCurrent [duration_ms]",
    handle_test_motor_current_command
};
