#pragma once

#include <Arduino.h>
#include "types/io_expander_pin.h"

enum class InputBackend : uint8_t
{
    ESP32_GPIO,
    INTERNAL_EXPANDER,
    EXTERNAL_EXPANDER
};

struct InputPin
{
    InputBackend backend;

    union
    {
        gpio_num_t esp32_pin;
        IoExpanderPin expander_pin;
    };

    bool read() const;
};
