#pragma once

#include <cstdint>
#include "types/input_pin.h"

class Input
{
public:
    Input(InputPin pin,
          bool active_low,
          uint32_t debounce_ms);

    void update(uint32_t now_ms);

    bool is_high() const;
    bool is_low() const;

    bool pressed();   // HIGH -> LOW (logical press)
    bool released();  // LOW -> HIGH (logical release)
    unsigned long get_stable_state_time();


private:
    InputPin pin;
    bool active_low;
    uint32_t debounce_ms;

    bool raw_state;
    bool stable_state;
    bool last_stable_state;

    uint32_t last_change_ms;

    bool pressed_event;
    bool released_event;

    bool normalize_state(bool value) const;
};
