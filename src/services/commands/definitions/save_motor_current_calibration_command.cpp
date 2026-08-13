#include "services/commands/command_definitions.h"

#include <cmath>
#include <cstdlib>

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

    bool parse_float_token(const char* token, float& value)
    {
        if (!token || *token == '\0') return false;

        char* parse_end = nullptr;
        value = strtof(token, &parse_end);
        return parse_end != token && *parse_end == '\0' && std::isfinite(value);
    }

    void log_usage()
    {
        LOG("Usage: saveMotorCurrentCalibration <rh|lh> <scale> <offset_a>");
    }

    void handle_save_motor_current_calibration_command(char* remaining_args)
    {
        if (!are_pop_ups_idle_or_timed_out()) {
            LOG("saveMotorCurrentCalibration rejected: pop-ups must be idle or timed out.");
            return;
        }

        char* cursor = remaining_args;
        char* scope_token = next_token(cursor);
        char* scale_token = next_token(cursor);
        char* offset_token = next_token(cursor);
        char* extra_token = next_token(cursor);
        if (!scope_token || !scale_token || !offset_token || extra_token) {
            LOG("saveMotorCurrentCalibration rejected: invalid arguments.");
            log_usage();
            return;
        }

        MotorCalibrationScope scope;
        const char* motor_name = nullptr;
        if (equals_ignore_case(scope_token, "rh")) {
            scope = MotorCalibrationScope::RH;
            motor_name = "RH";
        } else if (equals_ignore_case(scope_token, "lh")) {
            scope = MotorCalibrationScope::LH;
            motor_name = "LH";
        } else {
            LOG("saveMotorCurrentCalibration rejected: scope must be rh or lh.");
            log_usage();
            return;
        }

        float scale = 0.0f;
        float offset_a = 0.0f;
        if (!parse_float_token(scale_token, scale) ||
            !parse_float_token(offset_token, offset_a) ||
            scale <= 0.0f) {
            LOG("saveMotorCurrentCalibration rejected: scale must be positive and values must be finite.");
            log_usage();
            return;
        }

        const bool saved = save_motor_current_calibration(scope, scale, offset_a);
        LOG(
            "MOTOR_CAL_SAVE_RESULT motor=%s status=%s scale=%.6f offset_a=%.6f",
            motor_name,
            saved ? "ok" : "failed",
            scale,
            offset_a);
    }
}

extern const CommandDefinition kSaveMotorCurrentCalibrationCommandDefinition = {
    "saveMotorCurrentCalibration",
    "saveMotorCurrentCalibration <rh|lh> <scale> <offset_a>",
    handle_save_motor_current_calibration_command
};
