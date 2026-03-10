#include "services/commands/command_definitions.h"

#include <Arduino.h>

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

    void log_read_battery_voltage_usage()
    {
        LOG("Usage: readBatteryVoltage");
    }

    void handle_read_battery_voltage_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        if (next_token(cursor)) {
            LOG("readBatteryVoltage rejected: this command does not take arguments.");
            log_read_battery_voltage_usage();
            return;
        }

        constexpr uint8_t kReadCount = 5;
        constexpr uint16_t kDelayMs = 500;
        float voltage_sum = 0.0f;

        for (uint8_t i = 0; i < kReadCount; ++i) {
            const float battery_voltage = read_battery_voltage();
            voltage_sum += battery_voltage;
            LOG(
                "Battery voltage [%u/%u]: %.2f V",
                static_cast<unsigned>(i + 1),
                static_cast<unsigned>(kReadCount),
                battery_voltage);
            if (i + 1 < kReadCount) {
                delay(kDelayMs);
            }
        }

        const float voltage_average = voltage_sum / static_cast<float>(kReadCount);
        LOG(
            "Battery voltage average (%u readings): %.2f V",
            static_cast<unsigned>(kReadCount),
            voltage_average);
    }
}

extern const CommandDefinition kReadBatteryVoltageCommandDefinition = {
    "readBatteryVoltage",
    "readBatteryVoltage",
    handle_read_battery_voltage_command
};
