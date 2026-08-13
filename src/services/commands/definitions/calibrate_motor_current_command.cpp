#include "services/commands/command_definitions.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

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

    bool equals_ignore_case(const char* lhs, const char* rhs)
    {
        if (!lhs || !rhs) return false;

        size_t i = 0;
        while (lhs[i] != '\0' && rhs[i] != '\0') {
            char left = lhs[i];
            char right = rhs[i];
            if (left >= 'A' && left <= 'Z') left = static_cast<char>(left + ('a' - 'A'));
            if (right >= 'A' && right <= 'Z') right = static_cast<char>(right + ('a' - 'A'));
            if (left != right) return false;
            ++i;
        }
        return lhs[i] == '\0' && rhs[i] == '\0';
    }

    void log_usage()
    {
        LOG("Usage: calibrateMotorCurrent <rh|lh|both> [duration_ms]");
    }

    void handle_calibrate_motor_current_command(char* remaining_args)
    {
        if (!are_pop_ups_idle_or_timed_out()) {
            LOG("calibrateMotorCurrent rejected: pop-ups must be idle or timed out.");
            return;
        }

        char* cursor = remaining_args;
        char* scope_token = next_token(cursor);
        char* duration_token = next_token(cursor);
        char* extra_token = next_token(cursor);
        if (!scope_token || extra_token) {
            LOG("calibrateMotorCurrent rejected: expected a scope and optional duration.");
            log_usage();
            return;
        }

        uint32_t duration_ms = config::motors::drv8243::CALIBRATION_DURATION_MS;
        if (duration_token) {
            char* parse_end = nullptr;
            const unsigned long parsed_duration = strtoul(duration_token, &parse_end, 10);
            if (*duration_token == '\0' || !parse_end || *parse_end != '\0' ||
                parsed_duration == 0 || parsed_duration > UINT32_MAX) {
                LOG("calibrateMotorCurrent rejected: duration must be 1-%lu ms.",
                    static_cast<unsigned long>(UINT32_MAX));
                log_usage();
                return;
            }
            duration_ms = static_cast<uint32_t>(parsed_duration);
        }

        MotorCalibrationScope scope;
        if (equals_ignore_case(scope_token, "rh")) {
            scope = MotorCalibrationScope::RH;
        } else if (equals_ignore_case(scope_token, "lh")) {
            scope = MotorCalibrationScope::LH;
        } else if (equals_ignore_case(scope_token, "both")) {
            scope = MotorCalibrationScope::BOTH;
        } else {
            LOG("calibrateMotorCurrent rejected: invalid scope '%s'.", scope_token);
            log_usage();
            return;
        }

        LOG("calibrateMotorCurrent: duration=%lu ms per motor.",
            static_cast<unsigned long>(duration_ms));
        (void)calibrate_motor_current(scope, duration_ms);
    }
}

extern const CommandDefinition kCalibrateMotorCurrentCommandDefinition = {
    "calibrateMotorCurrent",
    "calibrateMotorCurrent <rh|lh|both> [duration_ms]",
    handle_calibrate_motor_current_command
};
