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

    void log_toggle_sleepy_eye_mode_usage()
    {
        LOG("Usage: toggleSleepyEyeMode");
    }

    void handle_toggle_sleepy_eye_mode_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        if (next_token(cursor))
        {
            LOG("toggleSleepyEyeMode rejected: this command does not take arguments.");
            log_toggle_sleepy_eye_mode_usage();
            return;
        }

        if (toggle_sleepy_eye_mode())
        {
            LOG("Command: toggleSleepyEyeMode applied.");
            return;
        }

        LOG("Command: toggleSleepyEyeMode was rejected by current safety rules.");
    }
}

extern const CommandDefinition kToggleSleepyEyeModeCommandDefinition = {
    "toggleSleepyEyeMode",
    "toggleSleepyEyeMode",
    handle_toggle_sleepy_eye_mode_command
};
