#include "verification/light_switch_test.h"

#include <Arduino.h>

#include "config.h"
#include "services/logging/logging.h"
#include "verification/io_expander_test.h"

namespace {
    bool s_last_hold_active = false;
    bool s_last_up_active = false;

    void set_active_low_led(IoExpanderPin pin, bool on)
    {
        write_internal_expander_led(pin, on);
    }

    void log_if_changed(const char* name, bool active, bool& last_active)
    {
        if (active == last_active) {
            return;
        }

        last_active = active;
        LOG("Light-switch %s input is %s.", name, active ? "LOW/active" : "HIGH/inactive");
    }
}

void setup_light_switch_test()
{
    pinMode(config::pins::LIGHT_SWITCH_HOLD_PIN, INPUT_PULLUP);
    pinMode(config::pins::LIGHT_SWITCH_UP_PIN, INPUT_PULLUP);

    LOG("Light-switch verification started. HOLD drives STATUS LED, UP drives ERROR LED.");
}

void update_light_switch_test()
{
    const bool hold_active = digitalRead(config::pins::LIGHT_SWITCH_HOLD_PIN) == LOW;
    const bool up_active = digitalRead(config::pins::LIGHT_SWITCH_UP_PIN) == LOW;

    set_active_low_led(config::pins::internal_expander::STATUS_LED_PIN, hold_active);
    set_active_low_led(config::pins::internal_expander::ERROR_LED_PIN, up_active);

    log_if_changed("HOLD", hold_active, s_last_hold_active);
    log_if_changed("UP", up_active, s_last_up_active);
}
