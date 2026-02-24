#include <Arduino.h>
#include <Preferences.h>
#include "helpers/pop_up.h"
#include "config.h"
#include "services/logging/logging.h"
#include "services/logging/error_log_manager.h"
#include "services/logging/statistics_manager.h"
#include "services/utilities/utilities.h"
#include "services/utilities/temperature.h"
#include <cstdarg>
#include <cstdio>


static Preferences error_log_preferences_storage;
static Preferences statistics_preferences_storage;
ErrorLogManager error_log_manager(error_log_preferences_storage);
StatisticsManager statistics_manager(statistics_preferences_storage);

void initialize_logging()
{
    statistics_manager.initialize();
    statistics_manager.increment_boot_count();
    error_log_manager.initialize();
}


void log_message(const char* fmt, ...)
{
    static constexpr size_t BUFFER_SIZE = 128;
    char buffer[BUFFER_SIZE];

    // Format user message
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, BUFFER_SIZE, fmt, args);
    va_end(args);

    // Print timestamp + message
    Serial.printf("[%lu] %s\n", millis(), buffer);
}

void report_pop_up_timeout(PopUpId pop_up_id)
{
    switch (pop_up_id)
    {
        case PopUpId::RH:
            report_error_code(ErrorCode::RH_POP_UP_TIMEOUT);
            break;

        case PopUpId::LH:
            report_error_code(ErrorCode::LH_POP_UP_TIMEOUT);
            break;
    }
}

void report_pop_up_overcurrent(PopUpId pop_up_id)
{
    switch (pop_up_id)
    {
        case PopUpId::RH:
            report_error_code(ErrorCode::RH_POP_UP_OVERCURRENT);
            break;

        case PopUpId::LH:
            report_error_code(ErrorCode::LH_POP_UP_OVERCURRENT);
            break;
    }
}


// Moved from services/pop_up_control/error_reporting.cpp
void report_error_code(ErrorCode code)
{
    // Placeholder implementation: create an ErrorLog entry and store it
    ErrorLog entry{};
    entry.timestamp_ms = millis();
    entry.boot_count = statistics_manager.get_boot_count();
    entry.error_code = code;
    entry.battery_voltage_mv = volts_to_mv(read_battery_voltage());
    entry.temperature_decic = celsius_to_decic(read_temperature());

    switch (code)
    {
        case ErrorCode::RH_POP_UP_TIMEOUT:
        case ErrorCode::RH_POP_UP_OVERCURRENT:
            statistics_manager.record_pop_up_error(PopUpId::RH);
            break;

        case ErrorCode::LH_POP_UP_TIMEOUT:
        case ErrorCode::LH_POP_UP_OVERCURRENT:
            statistics_manager.record_pop_up_error(PopUpId::LH);
            break;

        default:
            break;
    }

    error_log_manager.add_error_log_entry(entry);

    LOG("Reported error: %s", error_code_to_string(code));
}
