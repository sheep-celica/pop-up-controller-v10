#include "services/inputs/types/input.h"
#include "services/io/leds.h"
#include "services/io/power.h"


Input::Input(InputPin pin,
             bool active_low,
             uint32_t debounce_ms)
    : pin(pin),
      active_low(active_low),
      debounce_ms(debounce_ms),
      raw_state(false),
      stable_state(false),
      last_stable_state(false),
      last_change_ms(0),
      pressed_event(false),
      released_event(false)
{
    if (active_low && pin.backend == InputBackend::ESP32_GPIO)
    {
        pinMode(pin.esp32_pin, INPUT_PULLUP);
    }
}

void Input::set_pin(InputPin new_pin)
{
    pin = new_pin;

    if (active_low && pin.backend == InputBackend::ESP32_GPIO)
    {
        pinMode(pin.esp32_pin, INPUT_PULLUP);
    }

    // Reinitialize debounce history to the current electrical state so remapping
    // does not create a synthetic press/release event.
    const bool initial_state = normalize_state(pin.read());
    raw_state = initial_state;
    stable_state = initial_state;
    last_stable_state = initial_state;
    last_change_ms = millis();
    pressed_event = false;
    released_event = false;
}

bool Input::normalize_state(bool value) const
{
    return active_low ? !value : value;
}

void Input::update(uint32_t now_ms)
{
    constexpr float kInputChangeBlinkHz = 6.0f;

    pressed_event = false;
    released_event = false;

    bool new_raw = normalize_state(pin.read());

    if (new_raw != raw_state)
    {
        raw_state = new_raw;
        last_change_ms = now_ms;
    }

    if ((now_ms - last_change_ms) >= debounce_ms)
    {
        if (stable_state != raw_state)
        {
            last_stable_state = stable_state;
            stable_state = raw_state;
            blink_led(LedId::INPUT_LED, 1, kInputChangeBlinkHz);

            if (!last_stable_state && stable_state)
            {
                pressed_event = true;
                reset_idle_time();
            }
            else if (last_stable_state && !stable_state)
            {
                released_event = true;
            }
        }
    }
}

bool Input::is_high() const
{
    return stable_state;
}

bool Input::is_low() const
{
    return !stable_state;
}

bool Input::pressed()
{
    return pressed_event;
}

bool Input::released()
{
    return released_event;
}

unsigned long Input::get_stable_state_time()
{
    return millis()-last_change_ms;
}
