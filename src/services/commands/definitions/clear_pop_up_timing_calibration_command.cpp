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

    void handle_clear_pop_up_timing_calibration_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        if (next_token(cursor))
        {
            LOG("clearPopUptimingCalibration rejected: this command takes no arguments.");
            LOG("Usage: clearPopUptimingCalibration");
            return;
        }

        RH_POP_UP.timing_calibration.clear();
        LH_POP_UP.timing_calibration.clear();

        const bool rh_saved = RH_POP_UP.timing_calibration.save_to_preferences(RH_PREFS);
        const bool lh_saved = LH_POP_UP.timing_calibration.save_to_preferences(LH_PREFS);

        if (rh_saved && lh_saved)
        {
            LOG("Cleared RH/LH pop-up timing calibration and saved to NVS.");
            return;
        }

        if (!rh_saved && !lh_saved)
        {
            LOG("Cleared RH/LH timing calibration in RAM but failed to save both to NVS.");
            return;
        }

        if (!rh_saved)
        {
            LOG("Cleared RH/LH timing calibration in RAM, but failed to save RH to NVS.");
            return;
        }

        LOG("Cleared RH/LH timing calibration in RAM, but failed to save LH to NVS.");
    }
}

extern const CommandDefinition kClearPopUpTimingCalibrationCommandDefinition = {
    "clearPopUptimingCalibration",
    "clearPopUptimingCalibration",
    handle_clear_pop_up_timing_calibration_command
};
