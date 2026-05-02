#include "services/commands/command_definitions.h"

#include <cstring>

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
        LOG("Usage: readTemperature [hotspot|ambient|all]");
    }

    bool parse_temperature_sensor_role(char* token, TemperatureSensorRole& role)
    {
        if (!token || token[0] == '\0' || strcmp(token, "hotspot") == 0) {
            role = TemperatureSensorRole::Hotspot;
            return true;
        }

        if (strcmp(token, "ambient") == 0) {
            role = TemperatureSensorRole::Ambient;
            return true;
        }

        return false;
    }

    void log_temperature_read_result(TemperatureSensorRole role)
    {
        const TemperatureReadResult result = read_temperature(role);
        const char* role_name = temperature_sensor_role_name(role);

        if (!result.supported)
        {
            LOG("%s temperature: Not supported on this board.", role_name);
            return;
        }

        if (!result.connected)
        {
            LOG("%s temperature: Not Connected", role_name);
            return;
        }

        if (!result.read_ok)
        {
            LOG("%s temperature: Read Failed", role_name);
            return;
        }

        LOG("%s temperature: %.2f C", role_name, result.celsius);
    }

    void handle_read_temperature_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* sensor_token = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (extra_token)
        {
            LOG("readTemperature rejected: expected zero or one argument.");
            log_read_temperature_usage();
            return;
        }

        if (sensor_token && strcmp(sensor_token, "all") == 0)
        {
            log_temperature_read_result(TemperatureSensorRole::Hotspot);
            log_temperature_read_result(TemperatureSensorRole::Ambient);
            return;
        }

        TemperatureSensorRole role = TemperatureSensorRole::Hotspot;
        if (!parse_temperature_sensor_role(sensor_token, role))
        {
            LOG("readTemperature rejected: unknown sensor '%s'.", sensor_token);
            log_read_temperature_usage();
            return;
        }

        log_temperature_read_result(role);
    }
}

extern const CommandDefinition kReadTemperatureCommandDefinition = {
    "readTemperature",
    "readTemperature [hotspot|ambient|all]",
    handle_read_temperature_command
};
