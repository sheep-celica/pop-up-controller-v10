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

    void log_wink_usage()
    {
        LOG("Usage: wink <rh|lh|both>");
    }

    void handle_wink_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* scope_token = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (!scope_token || extra_token)
        {
            LOG("wink rejected: expected exactly one argument.");
            log_wink_usage();
            return;
        }

        if (equals_ignore_case(scope_token, "rh"))
        {
            LOG("Command: wink RH");
            RH_POP_UP.wink_pop_up();
            return;
        }

        if (equals_ignore_case(scope_token, "lh"))
        {
            LOG("Command: wink LH");
            LH_POP_UP.wink_pop_up();
            return;
        }

        if (equals_ignore_case(scope_token, "both"))
        {
            LOG("Command: wink both");
            RH_POP_UP.wink_pop_up();
            LH_POP_UP.wink_pop_up();
            return;
        }

        LOG("wink rejected: invalid scope '%s'.", scope_token);
        log_wink_usage();
    }
}

extern const CommandDefinition kWinkCommandDefinition = {
    "wink",
    "wink <rh|lh|both>",
    handle_wink_command
};
