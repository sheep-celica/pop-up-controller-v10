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

    void log_save_pop_up_timing_calibration_usage()
    {
        LOG("Usage: savePopUpTimingCalibration [rh|lh|both]");
    }

    bool save_rh_calibration()
    {
        const bool ok = RH_POP_UP.timing_calibration.save_to_preferences(RH_PREFS);
        if (ok) {
            LOG("RH pop-up timing calibration saved to NVS.");
        } else {
            LOG("Failed to save RH pop-up timing calibration to NVS.");
        }
        return ok;
    }

    bool save_lh_calibration()
    {
        const bool ok = LH_POP_UP.timing_calibration.save_to_preferences(LH_PREFS);
        if (ok) {
            LOG("LH pop-up timing calibration saved to NVS.");
        } else {
            LOG("Failed to save LH pop-up timing calibration to NVS.");
        }
        return ok;
    }

    void handle_save_pop_up_timing_calibration_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* scope_token = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (extra_token) {
            LOG("savePopUpTimingCalibration rejected: expected zero or one argument.");
            log_save_pop_up_timing_calibration_usage();
            return;
        }

        if (!scope_token || equals_ignore_case(scope_token, "both"))
        {
            const bool rh_ok = save_rh_calibration();
            const bool lh_ok = save_lh_calibration();
            if (rh_ok && lh_ok) {
                LOG("Both pop-up timing calibration tables were saved.");
            }
            return;
        }

        if (equals_ignore_case(scope_token, "rh")) {
            (void)save_rh_calibration();
            return;
        }

        if (equals_ignore_case(scope_token, "lh")) {
            (void)save_lh_calibration();
            return;
        }

        LOG("savePopUpTimingCalibration rejected: invalid scope '%s'.", scope_token);
        log_save_pop_up_timing_calibration_usage();
    }
}

extern const CommandDefinition kSavePopUpTimingCalibrationCommandDefinition = {
    "savePopUpTimingCalibration",
    "savePopUpTimingCalibration [rh|lh|both]",
    handle_save_pop_up_timing_calibration_command
};
