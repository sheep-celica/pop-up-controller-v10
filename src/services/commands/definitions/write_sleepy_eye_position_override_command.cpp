#include "services/commands/command_definitions.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "config.h"

namespace {
    char* next_token(char*& cursor)
    {
        if (!cursor) return nullptr;
        while (*cursor == ' ' || *cursor == '\t') ++cursor;
        if (*cursor == '\0') return nullptr;

        char* token = cursor;
        while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t') ++cursor;
        if (*cursor != '\0') *cursor++ = '\0';
        return token;
    }

    bool equals_ignore_case(const char* lhs, const char* rhs)
    {
        if (!lhs || !rhs) return false;
        while (*lhs && *rhs)
        {
            char a = (*lhs >= 'A' && *lhs <= 'Z') ? static_cast<char>(*lhs + ('a' - 'A')) : *lhs;
            char b = (*rhs >= 'A' && *rhs <= 'Z') ? static_cast<char>(*rhs + ('a' - 'A')) : *rhs;
            if (a != b) return false;
            ++lhs;
            ++rhs;
        }
        return *lhs == '\0' && *rhs == '\0';
    }

    bool parse_bool(const char* token, bool& value)
    {
        if (equals_ignore_case(token, "true") || equals_ignore_case(token, "on") ||
            (token && strcmp(token, "1") == 0)) { value = true; return true; }
        if (equals_ignore_case(token, "false") || equals_ignore_case(token, "off") ||
            (token && strcmp(token, "0") == 0)) { value = false; return true; }
        return false;
    }

    void log_usage()
    {
        LOG("Usage: writeSleepyEyePositionOverride <true|false> <0-6>");
    }

    void handle_write_sleepy_eye_position_override_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* enabled_token = next_token(cursor);
        char* position_token = next_token(cursor);
        char* extra_token = next_token(cursor);
        bool enabled = false;

        if (!enabled_token || !position_token || extra_token || !parse_bool(enabled_token, enabled))
        {
            LOG("writeSleepyEyePositionOverride rejected: expected a boolean and a position from 0 to 6.");
            log_usage();
            return;
        }

        errno = 0;
        char* end = nullptr;
        const long position = strtol(position_token, &end, 10);
        if (errno != 0 || end == position_token || *end != '\0' ||
            position < config::pop_up::SLEEPY_EYE_POSITION_MIN ||
            position > config::pop_up::SLEEPY_EYE_POSITION_MAX)
        {
            LOG("writeSleepyEyePositionOverride rejected: position must be from 0 to 6.");
            log_usage();
            return;
        }

        if (!set_sleepy_eye_position_override(enabled, static_cast<uint8_t>(position)))
        {
            LOG("writeSleepyEyePositionOverride failed: could not persist configuration to NVS.");
            return;
        }

        LOG(
            "SLEEPY_EYE_POSITION_OVERRIDE enabled=%s position=%u",
            is_sleepy_eye_position_override_enabled() ? "TRUE" : "FALSE",
            static_cast<unsigned>(get_sleepy_eye_position_override()));
    }
}

extern const CommandDefinition kWriteSleepyEyePositionOverrideCommandDefinition = {
    "writeSleepyEyePositionOverride",
    "writeSleepyEyePositionOverride <true|false> <0-6>",
    handle_write_sleepy_eye_position_override_command
};
