#include <Arduino.h>

#include "config.h"
#include "services/inputs/inputs_manager.h"
#include "services/inputs/logic/bh_button.h"
#include "services/inputs/logic/lh_button.h"
#include "services/inputs/logic/light_switch_hold.h"
#include "services/inputs/logic/light_switch_up.h"
#include "services/inputs/logic/rh_button.h"
#include "services/inputs/logic/sleepy_eye_button.h"
#include "services/inputs/logic/toggle_button.h"
#include "services/io/i2c_bus.h"
#include "services/io/io_expanders.h"
#include "services/io/leds.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"

#ifndef BUILD_VERSION
#define BUILD_VERSION "dev"
#endif

#ifndef BUILD_TIMESTAMP
#define BUILD_TIMESTAMP "unknown"
#endif

namespace {
    void set_shared_sleep_pin_high_early()
    {
        digitalWrite(config::motors::drv8243::SHARED_SLEEP_PIN, HIGH);
        pinMode(config::motors::drv8243::SHARED_SLEEP_PIN, OUTPUT);
    }

    void setup_shared_sleep_pin()
    {
        set_shared_sleep_pin_high_early();
        LOG("Shared SLEEP/nSLEEP GPIO %u set HIGH.", static_cast<unsigned>(config::motors::drv8243::SHARED_SLEEP_PIN));
    }

    void register_local_input_controls()
    {
        inputs_manager.reserve(8, 8);

        light_switch_up_register();
        light_switch_hold_register();

        rh_button_register();
        lh_button_register();
        bh_button_register();
        toggle_button_register();
        sleepy_eye_button_register();

        LOG(
            "Registered %d local inputs and %d input tasks.",
            inputs_manager.input_count(),
            inputs_manager.task_count());
    }
}

void setup()
{
    set_shared_sleep_pin_high_early();

    Serial.begin(115200);
    delay(300);

    initialize_logging(BUILD_VERSION, BUILD_TIMESTAMP);
    LOG("Running %s input/pop-up integration test firmware (%s).", config::board::HARDWARE_REVISION, config::board::ID);

    setup_shared_sleep_pin();
    setup_i2c_bus();
    setup_io_expanders();
    setup_pop_ups();
    setup_leds();
    register_local_input_controls();

    LOG("Input/pop-up integration test ready.");
    LOG("Power latch, idle power-off, bench mode, serial commands, remote inputs, and temperature services are disabled in this test firmware.");
}

void loop()
{
    inputs_manager.update();
    update_pop_ups();
    update_leds();
}
