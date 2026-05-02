#include "services/commands/command_definitions.h"

#include "services/io/io_expanders.h"
#include "services/io/power.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "services/utilities/temperature.h"
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

    void log_print_everything_usage()
    {
        LOG("Usage: printEverything");
    }

    void print_battery_voltage_calibration_summary()
    {
        float a = 1.0f;
        float b = 0.0f;
        const bool has_stored_calibration = get_battery_voltage_calibration(a, b);

        if (has_stored_calibration) {
            LOG("Battery voltage calibration constants: a=%.6f, b=%.6f", a, b);
            return;
        }

        LOG("Battery voltage calibration constants not fully stored. Effective values: a=%.6f, b=%.6f", a, b);
    }

    void print_idle_power_off_summary()
    {
        if (!is_idle_power_off_supported())
        {
            LOG("Idle shutdown: Not supported on this board.");
            return;
        }

        LOG(
            "Idle %s threshold: %lu s.",
            get_idle_power_action_name(),
            static_cast<unsigned long>(get_idle_time_to_power_off_seconds()));
    }

    void print_temperature_summary(TemperatureSensorRole role)
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

    void handle_print_everything_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* extra_token = next_token(cursor);
        if (extra_token) {
            LOG("printEverything rejected: this command does not take arguments.");
            log_print_everything_usage();
            return;
        }

        LOG("printEverything command received.");

        print_manufacture_data();
        statistics_manager.print_statistics();
        RH_POP_UP.timing_calibration.print_calibration("RH");
        LH_POP_UP.timing_calibration.print_calibration("LH");
        print_battery_voltage_calibration_summary();
        LOG(
            "ALLOW_SLEEPY_EYE_MODE_WITH_HEADLIGHTS=%s",
            is_sleepy_eye_mode_with_headlights_allowed() ? "TRUE" : "FALSE");
        print_idle_power_off_summary();
        log_fault_expander_status();
        error_log_manager.print_error_log_entries();
        print_temperature_summary(TemperatureSensorRole::Hotspot);
        print_temperature_summary(TemperatureSensorRole::Ambient);
        LOG("Battery voltage: %.2f V", read_battery_voltage());
    }
}

extern const CommandDefinition kPrintEverythingCommandDefinition = {
    "printEverything",
    "printEverything",
    handle_print_everything_command
};
