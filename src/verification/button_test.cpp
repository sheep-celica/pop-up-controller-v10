#include "verification/button_test.h"

#include <Arduino.h>

#include "services/logging/logging.h"
#include "verification/fault_expander_test.h"

namespace {
    constexpr uint8_t kRhButtonPin = 26;
    constexpr uint8_t kLhButtonPin = 25;
    constexpr uint8_t kSleepyButtonPin = 35;
    constexpr uint8_t kBhButtonExpanderPin = 6;
    constexpr uint8_t kToggleButtonExpanderPin = 7;

    struct ButtonState {
        const char* name;
        bool pressed;
        bool initialized;
    };

    ButtonState s_rh_button = { "RH", false, false };
    ButtonState s_lh_button = { "LH", false, false };
    ButtonState s_sleepy_button = { "Sleepy", false, false };
    ButtonState s_bh_button = { "BH", false, false };
    ButtonState s_toggle_button = { "Toggle", false, false };

    void log_button_if_changed(ButtonState& button, bool pressed)
    {
        if (button.initialized && button.pressed == pressed) {
            return;
        }

        button.initialized = true;
        button.pressed = pressed;
        LOG("Button %s is %s.", button.name, pressed ? "pressed/LOW" : "released/HIGH");
    }

    bool expander_pin_low(uint8_t states, uint8_t pin)
    {
        return ((states >> pin) & 0x01) == 0;
    }
}

void setup_button_test()
{
    pinMode(kRhButtonPin, INPUT_PULLUP);
    pinMode(kLhButtonPin, INPUT_PULLUP);
    pinMode(kSleepyButtonPin, INPUT);

    LOG("Button verification started.");
    LOG("GPIO %u and %u use ESP32 internal pullups.", kRhButtonPin, kLhButtonPin);
    LOG("GPIO %u has no ESP32 internal pullup; it needs the board external pullup.", kSleepyButtonPin);
    LOG("TCA6408A button inputs P%u/P%u use board external 10k pullups.",
        kBhButtonExpanderPin,
        kToggleButtonExpanderPin);
}

void update_button_test()
{
    log_button_if_changed(s_rh_button, digitalRead(kRhButtonPin) == LOW);
    log_button_if_changed(s_lh_button, digitalRead(kLhButtonPin) == LOW);
    log_button_if_changed(s_sleepy_button, digitalRead(kSleepyButtonPin) == LOW);

    uint8_t expander_states = 0;
    if (!read_fault_expander_inputs(expander_states)) {
        return;
    }

    log_button_if_changed(s_bh_button, expander_pin_low(expander_states, kBhButtonExpanderPin));
    log_button_if_changed(s_toggle_button, expander_pin_low(expander_states, kToggleButtonExpanderPin));
}
