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

    bool equals_ignore_case(const char* lhs, const char* rhs)
    {
        if (!lhs || !rhs) return false;

        size_t i = 0;
        while (lhs[i] != '\0' && rhs[i] != '\0')
        {
            const char lhs_char = (lhs[i] >= 'A' && lhs[i] <= 'Z')
                ? static_cast<char>(lhs[i] + ('a' - 'A'))
                : lhs[i];
            const char rhs_char = (rhs[i] >= 'A' && rhs[i] <= 'Z')
                ? static_cast<char>(rhs[i] + ('a' - 'A'))
                : rhs[i];

            if (lhs_char != rhs_char) {
                return false;
            }
            ++i;
        }

        return lhs[i] == '\0' && rhs[i] == '\0';
    }

    PopUpState get_both_toggle_target()
    {
        const bool both_down =
            RH_POP_UP.get_state() == PopUpState::DOWN &&
            LH_POP_UP.get_state() == PopUpState::DOWN;

        return both_down ? PopUpState::UP : PopUpState::DOWN;
    }

    void log_toggle_usage()
    {
        LOG("Usage: toggle <both>");
    }

    void handle_toggle_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* mode_token = next_token(cursor);

        if (!mode_token)
        {
            LOG("toggle rejected: missing mode argument.");
            log_toggle_usage();
            return;
        }

        if (equals_ignore_case(mode_token, "both"))
        {
            if (next_token(cursor))
            {
                LOG("toggle rejected: unexpected extra arguments for mode 'both'.");
                log_toggle_usage();
                return;
            }

            const PopUpState target = get_both_toggle_target();
            LOG("Command: toggle both -> %s", pop_up_state_name(target));
            safe_move_pop_up_to(&RH_POP_UP, target);
            safe_move_pop_up_to(&LH_POP_UP, target);
            return;
        }

        LOG("toggle rejected: invalid mode '%s'.", mode_token);
        log_toggle_usage();
    }
}

extern const CommandDefinition kToggleCommandDefinition = {
    "toggle",
    "toggle <both>",
    handle_toggle_command
};
