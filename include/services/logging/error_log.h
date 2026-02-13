#pragma once

#include <Arduino.h>
#include "services/logging/error_code.h"


struct ErrorLog 
{
    uint32_t  timestamp_ms;         // millis()
    uint32_t  boot_count;           // boot/session counter
    ErrorCode error_code;           // error identifier
    uint16_t  battery_voltage_mv;   // millivolts
    int16_t   temperature_decic;    // temperature × 10 (°C)
};

inline uint16_t volts_to_mv(float volts)
{
    return static_cast<uint16_t>(volts * 1000.0f);
}

inline int16_t celsius_to_decic(float celsius)
{
    return static_cast<int16_t>(celsius * 10.0f);
}

inline float mv_to_volts(uint16_t mv)
{
    return mv / 1000.0f;
}

inline float decic_to_celsius(int16_t decic)
{
    return decic / 10.0f;
}
