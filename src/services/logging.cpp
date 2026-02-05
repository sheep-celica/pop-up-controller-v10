#include <Arduino.h>
#include "services/logging.h"
#include <cstdarg>
#include <cstdio>

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
