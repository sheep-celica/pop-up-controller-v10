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

    bool try_parse_bool(char* token, bool& value)
    {
        if (!token || token[0] == '\0') {
            return false;
        }

        if (equals_ignore_case(token, "true") ||
            equals_ignore_case(token, "1") ||
            equals_ignore_case(token, "on"))
        {
            value = true;
            return true;
        }

        if (equals_ignore_case(token, "false") ||
            equals_ignore_case(token, "0") ||
            equals_ignore_case(token, "off"))
        {
            value = false;
            return true;
        }

        return false;
    }

    void log_write_sleepy_eye_mode_with_headlights_usage()
    {
        LOG("Usage: writeSleepyEyeModeWithHeadlights <true|false>");
    }

    void handle_write_sleepy_eye_mode_with_headlights_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* bool_token = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (!bool_token || extra_token)
        {
            LOG("writeSleepyEyeModeWithHeadlights rejected: expected exactly one argument.");
            log_write_sleepy_eye_mode_with_headlights_usage();
            return;
        }

        bool requested_value = false;
        if (!try_parse_bool(bool_token, requested_value))
        {
            LOG("writeSleepyEyeModeWithHeadlights rejected: expected true/false.");
            log_write_sleepy_eye_mode_with_headlights_usage();
            return;
        }

        if (!set_sleepy_eye_mode_with_headlights_allowed(requested_value))
        {
            LOG("writeSleepyEyeModeWithHeadlights failed: could not persist value to NVS.");
            return;
        }

        LOG(
            "ALLOW_SLEEPY_EYE_MODE_WITH_HEADLIGHTS set to %s.",
            is_sleepy_eye_mode_with_headlights_allowed() ? "TRUE" : "FALSE");
    }
}

extern const CommandDefinition kWriteSleepyEyeModeWithHeadlightsCommandDefinition = {
    "writeSleepyEyeModeWithHeadlights",
    "writeSleepyEyeModeWithHeadlights <true|false>",
    handle_write_sleepy_eye_mode_with_headlights_command
};
