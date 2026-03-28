#include "services/commands/command_definitions.h"

#include "services/logging/logging.h"
#include "services/utilities/temperature.h"

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

    void log_read_temperature_usage()
    {
        LOG("Usage: readTemperature");
    }

    void handle_read_temperature_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        if (next_token(cursor))
        {
            LOG("readTemperature rejected: this command does not take arguments.");
            log_read_temperature_usage();
            return;
        }

        if (!is_temperature_sensor_connected())
        {
            LOG("Temperature: Not Connected");
            return;
        }

        LOG("Temperature: %.2f C", read_temperature());
    }
}

extern const CommandDefinition kReadTemperatureCommandDefinition = {
    "readTemperature",
    "readTemperature",
    handle_read_temperature_command
};
