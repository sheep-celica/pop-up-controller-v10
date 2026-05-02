#include <Arduino.h>

#include "config.h"
#include "services/commands/commands.h"
#include "services/inputs/inputs_manager.h"
#include "services/inputs/register_inputs.h"
#include "services/io/i2c_bus.h"
#include "services/io/io_expanders.h"
#include "services/io/leds.h"
#include "services/io/power.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "services/utilities/controller_status.h"
#include "services/utilities/temperature.h"
#include "services/utilities/utilities.h"

#ifndef BUILD_VERSION
#define BUILD_VERSION "dev"
#endif

#ifndef BUILD_TIMESTAMP
#define BUILD_TIMESTAMP "unknown"
#endif

namespace {
    void set_shared_sleep_pin_high_early()
    {
        if (!config::features::HAS_DRV8243_MOTOR_DRIVER)
        {
            return;
        }

        digitalWrite(config::pins::motor_driver::SHARED_SLEEP_PIN, HIGH);
        pinMode(config::pins::motor_driver::SHARED_SLEEP_PIN, OUTPUT);
    }

    void setup_shared_sleep_pin()
    {
        if (!config::features::HAS_DRV8243_MOTOR_DRIVER)
        {
            return;
        }

        set_shared_sleep_pin_high_early();
        LOG("Shared SLEEP/nSLEEP GPIO %u set HIGH.", static_cast<unsigned>(config::pins::motor_driver::SHARED_SLEEP_PIN));
    }
}

void setup()
{
    setup_power();
    set_shared_sleep_pin_high_early();

    Serial.begin(115200);

    initialize_logging(BUILD_VERSION, BUILD_TIMESTAMP);
    LOG("Running %s firmware (%s).", config::board::HARDWARE_REVISION, config::board::ID);

    setup_shared_sleep_pin();
    setup_i2c_bus();
    setup_io_expanders();
    setup_pop_ups();
    setup_leds();
    register_inputs();

    const float battery_voltage = read_battery_voltage();
    const bool bench_mode_enabled = initialize_controller_bench_mode(battery_voltage);
    log_startup_summary(battery_voltage, bench_mode_enabled);
    setup_temperature();

    LOG("Firmware startup complete.");
}

void loop()
{
    const uint32_t now_ms = millis();

    update_external_expander_runtime_state();
    update_fault_expander_runtime_state();
    if (!is_controller_bench_mode_enabled())
    {
        inputs_manager.update();
        update_pop_ups();
        update_remote_input_registration();
        check_idle_time();
    }

    update_bench_mode_led_indicator(now_ms);
    update_leds();
    statistics_manager.update_runtime();
    update_commands();
}
