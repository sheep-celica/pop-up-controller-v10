#include "services/commands/command_definitions.h"

#include <cstring>

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

    char to_lower_ascii(char c)
    {
        if (c >= 'A' && c <= 'Z') {
            return static_cast<char>(c + ('a' - 'A'));
        }
        return c;
    }

    bool equals_ignore_case(const char* lhs, const char* rhs)
    {
        if (!lhs || !rhs) return false;

        size_t i = 0;
        while (lhs[i] != '\0' && rhs[i] != '\0')
        {
            if (to_lower_ascii(lhs[i]) != to_lower_ascii(rhs[i])) {
                return false;
            }
            ++i;
        }

        return lhs[i] == '\0' && rhs[i] == '\0';
    }

    void log_print_pop_up_timing_calibration_usage()
    {
        LOG("Usage: printPopUpTimingCalibration [rh|lh|both]");
    }

    void handle_print_pop_up_timing_calibration_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* scope_token = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (extra_token) {
            LOG("printPopUpTimingCalibration rejected: expected zero or one argument.");
            log_print_pop_up_timing_calibration_usage();
            return;
        }

        if (!scope_token || equals_ignore_case(scope_token, "both"))
        {
            RH_POP_UP.timing_calibration.print_calibration("RH");
            LH_POP_UP.timing_calibration.print_calibration("LH");
            return;
        }

        if (equals_ignore_case(scope_token, "rh")) {
            RH_POP_UP.timing_calibration.print_calibration("RH");
            return;
        }

        if (equals_ignore_case(scope_token, "lh")) {
            LH_POP_UP.timing_calibration.print_calibration("LH");
            return;
        }

        LOG("printPopUpTimingCalibration rejected: invalid scope '%s'.", scope_token);
        log_print_pop_up_timing_calibration_usage();
    }
}

extern const CommandDefinition kPrintPopUpTimingCalibrationCommandDefinition = {
    "printPopUpTimingCalibration",
    "printPopUpTimingCalibration [rh|lh|both]",
    handle_print_pop_up_timing_calibration_command
};
