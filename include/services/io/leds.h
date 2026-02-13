#pragma once

#include <cstdint>
#include "services/io/types/io_expander_pin.h"
#include "config.h"

// Identifiers for the on/off (digital) LEDs on the internal expander
// Avoid common macro names (e.g. INPUT) by suffixing with _LED
enum class LedId : uint8_t {
    STATUS_LED = 0,
    INPUT_LED  = 1,
    ERROR_LED  = 2
};

// Initialize LED GPIOs and PWM for illumination
void setup_leds();

// Set one of the three digital LEDs on or off
void set_led_state(LedId led, bool on);

// Control the PWM illumination LED (on-board, GPIO) — uses pot to set duty
void turn_on_illumination();
void turn_off_illumination();
