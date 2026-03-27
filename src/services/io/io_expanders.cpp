#include "services/io/io_expanders.h"
#include "config.h"
#include "services/io/leds.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"

namespace {
    constexpr uint8_t kExternalExpanderInactiveInputs = 0xFF;

    bool s_external_expander_connected = false;
    bool s_external_expander_disconnect_latched = false;
    uint8_t s_external_expander_i2c_address = config::pins::external_expander::DEFAULT_I2C_ADDRESS;
    uint32_t s_next_external_expander_probe_ms = 0;
    bool s_external_expander_input_cache_valid = false;
    uint8_t s_external_expander_input_cache = kExternalExpanderInactiveInputs;

    void invalidate_external_expander_input_cache()
    {
        s_external_expander_input_cache_valid = false;
        s_external_expander_input_cache = kExternalExpanderInactiveInputs;
    }

    bool refresh_external_expander_input_cache()
    {
        s_external_expander_input_cache = remote_pcf.read8();

        if (remote_pcf.lastError() != PCF8574_OK)
        {
            invalidate_external_expander_input_cache();
            return false;
        }

        s_external_expander_input_cache_valid = true;
        return true;
    }

    bool try_begin_external_expander_at_address(uint8_t address)
    {
        return remote_pcf.setAddress(address) && remote_pcf.begin();
    }

    bool try_probe_external_expander_at_address(uint8_t address)
    {
        return remote_pcf.setAddress(address);
    }

    bool try_probe_external_expander()
    {
        const uint8_t primary_external_address = config::pins::external_expander::DEFAULT_I2C_ADDRESS;
        const uint8_t fallback_external_address = config::pins::external_expander::FALLBACK_I2C_ADDRESS;

        if (try_probe_external_expander_at_address(primary_external_address))
        {
            return true;
        }

        return fallback_external_address != primary_external_address &&
               try_probe_external_expander_at_address(fallback_external_address);
    }

    void latch_external_expander_disconnect()
    {
        if (s_external_expander_disconnect_latched)
        {
            return;
        }

        s_external_expander_connected = false;
        s_external_expander_disconnect_latched = true;
        invalidate_external_expander_input_cache();
        report_error_code(ErrorCode::REMOTE_EXPANDER_DISCONNECTED);
        set_led_state(LedId::ERROR_LED, true);
        LOG("External expander disconnected during runtime. Remote inputs disabled until power cycle.");
    }
}


// Initiate the helper classes
PCF8574 remote_pcf(config::pins::external_expander::DEFAULT_I2C_ADDRESS);
ADS7138 internal_ads(config::pins::internal_expander::I2C_ADDRESS);

void setup_io_expanders()
{
    const uint8_t primary_external_address = config::pins::external_expander::DEFAULT_I2C_ADDRESS;
    const uint8_t fallback_external_address = config::pins::external_expander::FALLBACK_I2C_ADDRESS;

    bool external_expander_connected = try_begin_external_expander_at_address(primary_external_address);
    if (!external_expander_connected && fallback_external_address != primary_external_address)
    {
        external_expander_connected = try_begin_external_expander_at_address(fallback_external_address);
    }

    s_external_expander_connected = external_expander_connected;
    s_external_expander_disconnect_latched = false;
    s_external_expander_i2c_address = remote_pcf.getAddress();
    s_next_external_expander_probe_ms = 0;
    invalidate_external_expander_input_cache();

    if (external_expander_connected)
    {
        LOG("External expander detected at I2C address 0x%02X.", remote_pcf.getAddress());
    }
    else
    {
        LOG(
            "External expander not detected at I2C addresses 0x%02X or 0x%02X.",
            primary_external_address,
            fallback_external_address);
    }

    // Ugly I know, but setting up pin configurations of the internal ADS7138 here
    internal_ads.setAnalogInput(    static_cast<uint8_t> (config::pins::internal_expander::BATTERY_VOLTAGE_PIN      ));
    internal_ads.setAnalogInput(    static_cast<uint8_t> (config::pins::internal_expander::LED_ADJUST_POT_PIN       ));
    internal_ads.setAnalogInput(    static_cast<uint8_t> (config::pins::internal_expander::POP_UP_OFFSET_POT_PIN    ));
    internal_ads.setDigitalOutput(  static_cast<uint8_t> (config::pins::internal_expander::INPUT_LED_PIN            ), true);
    internal_ads.setDigitalOutput(  static_cast<uint8_t> (config::pins::internal_expander::ERROR_LED_PIN            ), true);
    internal_ads.setDigitalOutput(  static_cast<uint8_t> (config::pins::internal_expander::STATUS_LED_PIN           ), true);
    internal_ads.setDigitalOutput(  static_cast<uint8_t> (config::pins::internal_expander::SLEEPY_EYE_LED_PIN       ), true);
    internal_ads.setDigitalInput(   static_cast<uint8_t> (config::pins::internal_expander::DEBUG_BUTTON_PIN         ));

    // Beginning the IO expanders
    internal_ads.begin();
}

void update_external_expander_runtime_state()
{
    if (s_external_expander_disconnect_latched || !are_pop_ups_idle_or_timed_out())
    {
        return;
    }

    const uint32_t now_ms = millis();
    if (now_ms < s_next_external_expander_probe_ms)
    {
        return;
    }

    s_next_external_expander_probe_ms =
        now_ms + config::pins::external_expander::RUNTIME_PROBE_INTERVAL_MS;

    if (s_external_expander_connected)
    {
        if (refresh_external_expander_input_cache())
        {
            return;
        }

        latch_external_expander_disconnect();
        return;
    }

    if (!try_probe_external_expander())
    {
        return;
    }

    if (!remote_pcf.begin())
    {
        return;
    }

    s_external_expander_connected = true;
    s_external_expander_i2c_address = remote_pcf.getAddress();
    invalidate_external_expander_input_cache();
    (void)refresh_external_expander_input_cache();
    LOG(
        "External expander detected at I2C address 0x%02X during runtime.",
        static_cast<unsigned>(s_external_expander_i2c_address));
}

bool is_external_expander_connected()
{
    return s_external_expander_connected;
}

uint8_t get_external_expander_i2c_address()
{
    return s_external_expander_i2c_address;
}

bool read_external_expander_pin(IoExpanderPin pin)
{
    if (!s_external_expander_connected || !s_external_expander_input_cache_valid)
    {
        return true;
    }

    const uint8_t bit = static_cast<uint8_t>(pin);
    return (s_external_expander_input_cache & (1u << bit)) != 0u;
}

