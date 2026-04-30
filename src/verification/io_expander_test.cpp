#include "verification/io_expander_test.h"

#include <Arduino.h>

#include "config.h"
#include "helpers/ADS7138.h"
#include "services/logging/logging.h"

namespace {
    ADS7138 s_internal_ads(config::pins::internal_expander::I2C_ADDRESS);

    constexpr uint32_t kLedTestDurationMs = 1000;

    uint8_t expander_pin(IoExpanderPin pin)
    {
        return static_cast<uint8_t>(pin);
    }

    bool probe_internal_expander()
    {
        Wire.beginTransmission(config::pins::internal_expander::I2C_ADDRESS);
        return Wire.endTransmission() == 0;
    }

    void configure_internal_expander_pins()
    {
        s_internal_ads.setAnalogInput(expander_pin(config::pins::internal_expander::BATTERY_VOLTAGE_PIN));
        s_internal_ads.setDigitalOutput(expander_pin(config::pins::internal_expander::SLEEPY_EYE_LED_PIN), true);
        s_internal_ads.setAnalogInput(expander_pin(config::pins::internal_expander::LED_ADJUST_POT_PIN));
        s_internal_ads.setAnalogInput(3);
        s_internal_ads.setDigitalInput(expander_pin(config::pins::internal_expander::DEBUG_BUTTON_PIN));
        s_internal_ads.setDigitalOutput(expander_pin(config::pins::internal_expander::INPUT_LED_PIN), true);
        s_internal_ads.setDigitalOutput(expander_pin(config::pins::internal_expander::ERROR_LED_PIN), true);
        s_internal_ads.setDigitalOutput(expander_pin(config::pins::internal_expander::STATUS_LED_PIN), true);
    }

    void write_active_low_leds(bool on)
    {
        const bool pin_level = !on;
        s_internal_ads.digitalWrite(expander_pin(config::pins::internal_expander::INPUT_LED_PIN), pin_level);
        s_internal_ads.digitalWrite(expander_pin(config::pins::internal_expander::ERROR_LED_PIN), pin_level);
        s_internal_ads.digitalWrite(expander_pin(config::pins::internal_expander::STATUS_LED_PIN), pin_level);
    }

    void print_raw_analog_values()
    {
        const uint8_t battery_pin = expander_pin(config::pins::internal_expander::BATTERY_VOLTAGE_PIN);
        const uint8_t pot_pin = expander_pin(config::pins::internal_expander::LED_ADJUST_POT_PIN);

        LOG(
            "Internal ADS7138 raw analog: battery pin %u = %u, potentiometer pin %u = %u.",
            battery_pin,
            s_internal_ads.readAnalogRaw(battery_pin),
            pot_pin,
            s_internal_ads.readAnalogRaw(pot_pin));
    }
}

bool setup_io_expander_test()
{
    const bool present = probe_internal_expander();
    LOG(
        "Internal ADS7138 expander at 0x%02X: %s.",
        config::pins::internal_expander::I2C_ADDRESS,
        present ? "present" : "not present");

    if (!present) {
        return false;
    }

    configure_internal_expander_pins();
    if (!s_internal_ads.begin()) {
        LOG("Internal ADS7138 setup failed.");
        return false;
    }

    LOG("Internal ADS7138 setup complete. Testing active-low INPUT/ERROR/STATUS LEDs.");
    print_raw_analog_values();
    write_active_low_leds(true);
    delay(kLedTestDurationMs);
    write_active_low_leds(false);
    LOG("Internal ADS7138 LED test complete.");

    return true;
}

void write_internal_expander_led(IoExpanderPin pin, bool on)
{
    s_internal_ads.digitalWrite(expander_pin(pin), !on);
}

void blink_internal_expander_led(IoExpanderPin pin, uint32_t duration_ms)
{
    write_internal_expander_led(pin, true);
    delay(duration_ms);
    write_internal_expander_led(pin, false);
}
