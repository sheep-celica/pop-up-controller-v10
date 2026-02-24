#include "services/commands/command_definitions.h"

#include <cstring>

#include "services/logging/logging.h"

namespace {
    constexpr const char* CLEAR_STATS_PASSWORD = "iKnowWhatImDoing";

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

    void handle_clear_statistical_data(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* password = next_token(cursor);

        if (!password) {
            LOG("clearStatisticalData rejected: missing password argument.");
            LOG("Usage: clearStatisticalData <password>");
            return;
        }

        if (strcmp(password, CLEAR_STATS_PASSWORD) != 0) {
            LOG("clearStatisticalData rejected: incorrect password.");
            return;
        }

        if (!statistics_manager.clear_all_statistics()) {
            LOG("clearStatisticalData failed: statistics manager not initialized or NVS clear failed.");
            return;
        }

        LOG("clearStatisticalData succeeded: all statistical data cleared.");
    }
}

extern const CommandDefinition kClearStatisticalDataCommandDefinition = {
    "clearStatisticalData",
    "clearStatisticalData <password>",
    handle_clear_statistical_data
};
