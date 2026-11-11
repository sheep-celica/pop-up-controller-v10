#include "services/commands/command_definitions.h"

#include <cstring>

#include "services/logging/logging.h"
#include "services/utilities/controller_status.h"

namespace {
    bool is_space_char(char c)
    {
        return c == ' ' || c == '\t';
    }

    char* next_token(char*& cursor)
    {
        if (!cursor) return nullptr;
        while (*cursor != '\0' && is_space_char(*cursor)) ++cursor;
        if (*cursor == '\0') return nullptr;

        char* token = cursor;
        while (*cursor != '\0' && !is_space_char(*cursor)) ++cursor;
        if (*cursor != '\0')
        {
            *cursor = '\0';
            ++cursor;
        }
        return token;
    }

    bool parse_bool(const char* token, bool& value)
    {
        if (!token) return false;
        if (strcmp(token, "true") == 0)
        {
            value = true;
            return true;
        }
        if (strcmp(token, "false") == 0)
        {
            value = false;
            return true;
        }
        return false;
    }

    void handle_write_forced_bench_mode_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* enabled_token = next_token(cursor);
        char* extra_token = next_token(cursor);
        bool enabled = false;
        if (!enabled_token || extra_token || !parse_bool(enabled_token, enabled))
        {
            LOG("writeForcedBenchMode rejected: expected true or false.");
            LOG("Usage: writeForcedBenchMode <true|false>");
            return;
        }

        if (!save_forced_bench_mode_configuration(enabled))
        {
            LOG("writeForcedBenchMode failed: could not persist configuration to NVS.");
            return;
        }

        LOG(
            "Forced bench-mode lock %s. Reboot required before the new setting takes effect.",
            enabled ? "enabled" : "disabled");
        LOG(
            "FORCED_BENCH_MODE configured=%s active=%s reboot_required=%s",
            is_forced_bench_mode_configured() ? "true" : "false",
            is_forced_bench_mode_active() ? "true" : "false",
            is_forced_bench_mode_configured() != is_forced_bench_mode_active()
                ? "true"
                : "false");
    }
}

extern const CommandDefinition kWriteForcedBenchModeCommandDefinition = {
    "writeForcedBenchMode",
    "writeForcedBenchMode <true|false>",
    handle_write_forced_bench_mode_command
};
