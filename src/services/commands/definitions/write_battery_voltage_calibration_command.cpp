#include "services/commands/command_definitions.h"

#include <cstdlib>

#include "services/logging/logging.h"
#include "services/utilities/utilities.h"

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

    bool try_parse_float(char* token, float& value)
    {
        if (!token || token[0] == '\0') {
            return false;
        }

        char* end = nullptr;
        value = strtof(token, &end);
        return end != token && end && *end == '\0';
    }

    void log_write_battery_voltage_calibration_usage()
    {
        LOG("Usage: writeBatteryVoltageCalibration <a> <b>");
    }

    void handle_write_battery_voltage_calibration_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* a_token = next_token(cursor);
        char* b_token = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (!a_token || !b_token || extra_token) {
            LOG("writeBatteryVoltageCalibration rejected: expected exactly two numeric arguments.");
            log_write_battery_voltage_calibration_usage();
            return;
        }

        float a = 0.0f;
        float b = 0.0f;
        if (!try_parse_float(a_token, a) || !try_parse_float(b_token, b)) {
            LOG("writeBatteryVoltageCalibration rejected: invalid numeric argument.");
            log_write_battery_voltage_calibration_usage();
            return;
        }

        if (!write_battery_voltage_calibration(a, b)) {
            LOG("writeBatteryVoltageCalibration failed: could not persist calibration constants.");
            return;
        }

        float stored_a = a;
        float stored_b = b;
        (void)get_battery_voltage_calibration(stored_a, stored_b);

        LOG("Battery voltage calibration updated.");
        LOG("Battery voltage calibration constants: a=%.6f, b=%.6f", stored_a, stored_b);
    }
}

extern const CommandDefinition kWriteBatteryVoltageCalibrationCommandDefinition = {
    "writeBatteryVoltageCalibration",
    "writeBatteryVoltageCalibration <a> <b>",
    handle_write_battery_voltage_calibration_command
};
