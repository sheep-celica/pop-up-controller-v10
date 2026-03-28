#pragma once

#include <Arduino.h>


enum class ErrorCode : uint8_t 
{
    RH_POP_UP_TIMEOUT,
    RH_POP_UP_OVERCURRENT,
    LH_POP_UP_TIMEOUT,
    LH_POP_UP_OVERCURRENT,
    HIGH_TEMPERATURE,
    CRITICAL_TEMPERATURE,
    LOW_BATTERY_VOLTAGE,
    TEMP_SENSOR_MALFUNCTION,
    REMOTE_EXPANDER_DISCONNECTED
};

inline const char* error_code_to_string(ErrorCode code)
{
    switch (code) {
        case ErrorCode::RH_POP_UP_TIMEOUT:
            return "RH_POP_UP_TIMEOUT";

        case ErrorCode::RH_POP_UP_OVERCURRENT:
            return "RH_POP_UP_OVERCURRENT";

        case ErrorCode::LH_POP_UP_TIMEOUT:
            return "LH_POP_UP_TIMEOUT";

        case ErrorCode::LH_POP_UP_OVERCURRENT:
            return "LH_POP_UP_OVERCURRENT";

        case ErrorCode::HIGH_TEMPERATURE:
            return "HIGH_TEMPERATURE";

        case ErrorCode::CRITICAL_TEMPERATURE:
            return "CRITICAL_TEMPERATURE";

        case ErrorCode::LOW_BATTERY_VOLTAGE:
            return "LOW_BATTERY_VOLTAGE";

        case ErrorCode::TEMP_SENSOR_MALFUNCTION:
            return "TEMP_SENSOR_MALFUNCTION";

        case ErrorCode::REMOTE_EXPANDER_DISCONNECTED:
            return "REMOTE_EXPANDER_DISCONNECTED";

        default:
            return "UNKNOWN_ERROR";
    }
}
