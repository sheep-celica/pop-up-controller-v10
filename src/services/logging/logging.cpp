#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "services/logging/logging.h"
#include "services/logging/error_log_manager.h"
#include <cstdarg>
#include <cstdio>


static Preferences preferences_storage;
ErrorLogManager error_log_manager(preferences_storage);

void initialize_logging()
{
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


// Moved from services/pop_up_control/error_reporting.cpp
void report_error_code(ErrorCode code)
{
    // Placeholder implementation: create an ErrorLog entry and store it
    ErrorLog entry{};
    entry.timestamp_ms = millis();
    entry.boot_count = 0; // TODO: supply real boot count
    entry.error_code = code;
    entry.battery_voltage_mv = 0; // TODO: read battery
    entry.temperature_decic = 0; // TODO: read temperature

    error_log_manager.add_error_log_entry(entry);

    LOG("Reported error: %s", error_code_to_string(code));
}
