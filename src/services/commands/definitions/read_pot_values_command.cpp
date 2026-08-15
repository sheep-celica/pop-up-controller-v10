#include "services/commands/command_definitions.h"

#include <Arduino.h>

#include "config.h"
#include "services/io/io_expanders.h"
#include "services/logging/logging.h"

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

    void log_read_pot_values_usage()
    {
        LOG("Usage: readPotValues");
    }

    float clamp_pot_ratio(float volts)
    {
        float ratio = volts / 3.3f;
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;
        return ratio;
    }

    void handle_read_pot_values_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        if (next_token(cursor)) {
            LOG("readPotValues rejected: this command does not take arguments.");
            log_read_pot_values_usage();
            return;
        }

        const float led_adjust_volts = internal_ads.readAnalogVolts(
            static_cast<uint8_t>(config::pins::internal_expander::LED_ADJUST_POT_PIN));
        const float led_adjust_percent = clamp_pot_ratio(led_adjust_volts) * 100.0f;

        if (config::features::HAS_RH_POP_UP_OFFSET_POT)
        {
            const float rh_offset_volts = internal_ads.readAnalogVolts(
                static_cast<uint8_t>(config::pins::internal_expander::POP_UP_OFFSET_POT_PIN));
            const float rh_offset_ratio = clamp_pot_ratio(rh_offset_volts);
            const int rh_offset_ms = static_cast<int>(
                rh_offset_ratio * config::pop_up::RH_POP_UP_OFFSET_RANGE_MS * 2.0f -
                config::pop_up::RH_POP_UP_OFFSET_RANGE_MS);

            LOG(
                "POT_VALUES rh_offset_supported=true rh_offset_volts=%.3f rh_offset_ms=%d "
                "led_adjust_supported=true led_adjust_volts=%.3f led_adjust_percent=%.1f",
                rh_offset_volts,
                rh_offset_ms,
                led_adjust_volts,
                led_adjust_percent);
            LOG(
                "RH offset pot: %.3f V -> %d ms (-%lu to +%lu ms)",
                rh_offset_volts,
                rh_offset_ms,
                static_cast<unsigned long>(config::pop_up::RH_POP_UP_OFFSET_RANGE_MS),
                static_cast<unsigned long>(config::pop_up::RH_POP_UP_OFFSET_RANGE_MS));
        }
        else
        {
            LOG(
                "POT_VALUES rh_offset_supported=false led_adjust_supported=true "
                "led_adjust_volts=%.3f led_adjust_percent=%.1f",
                led_adjust_volts,
                led_adjust_percent);
            LOG("RH offset pot: Not supported on this board.");
        }

        LOG("LED adjust pot: %.3f V -> %.1f%%", led_adjust_volts, led_adjust_percent);
    }
}

extern const CommandDefinition kReadPotValuesCommandDefinition = {
    "readPotValues",
    "readPotValues",
    handle_read_pot_values_command
};
