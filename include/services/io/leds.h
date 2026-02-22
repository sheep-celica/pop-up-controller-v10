#pragma once

#include <cstdint>
#include "services/io/types/io_expander_pin.h"
#include "config.h"

// Identifiers for the on/off (digital) LEDs on the internal expander
// Avoid common macro names (e.g. INPUT) by suffixing with _LED
enum class LedId : uint8_t {
    STATUS_LED = 0,
    INPUT_LED  = 1,
    ERROR_LED  = 2,
    SLEEPY_EYE_STATUS = 3
};

// Initialize LED GPIOs and PWM for illumination
void setup_leds();

// Set a digital LED on or off
void set_led_state(LedId led, bool on);

// Blink a digital LED `times` times at `frequency_hz`.
// If the same LED is already blinking, the new request is added to the remaining blink count.
void blink_led(LedId led, uint32_t times, float frequency_hz);

// Control the PWM illumination LED (on-board, GPIO) — uses pot to set duty
void turn_on_illumination();
void turn_off_illumination();
// Called from main loop to advance any ramps and keep LED state updated
void update_leds();
