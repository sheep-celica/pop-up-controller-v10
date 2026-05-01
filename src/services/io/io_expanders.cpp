#include "services/io/io_expanders.h"
#include "config.h"
#include "services/io/leds.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"

#include <Wire.h>

namespace {
    constexpr uint8_t kExternalExpanderInactiveInputs = 0xFF;
    constexpr uint8_t kFaultExpanderInactiveInputs = 0xFF;
    constexpr uint8_t kTca6408aInputPortRegister = 0x00;
    constexpr uint8_t kTca6408aPolarityInversionRegister = 0x02;
    constexpr uint8_t kTca6408aConfigurationRegister = 0x03;

    bool s_external_expander_connected = false;
    bool s_external_expander_disconnect_latched = false;
    uint8_t s_external_expander_i2c_address = config::pins::external_expander::DEFAULT_I2C_ADDRESS;
    uint32_t s_next_external_expander_probe_ms = 0;
    bool s_external_expander_input_cache_valid = false;
    uint8_t s_external_expander_input_cache = kExternalExpanderInactiveInputs;

    bool s_fault_expander_connected = false;
    bool s_fault_expander_input_cache_valid = false;
    uint8_t s_fault_expander_input_cache = kFaultExpanderInactiveInputs;

    void invalidate_external_expander_input_cache()
    {
        s_external_expander_input_cache_valid = false;
        s_external_expander_input_cache = kExternalExpanderInactiveInputs;
    }

    bool refresh_external_expander_input_cache()
    {
        s_external_expander_input_cache = remote_pcf.read8();
        const int error_code = remote_pcf.lastError();

        if (error_code != PCF8574_OK)
        {
            invalidate_external_expander_input_cache();
            return false;
        }

        s_external_expander_input_cache_valid = true;
        return true;
    }

    bool probe_current_external_expander_connection()
    {
        return remote_pcf.isConnected();
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

    void invalidate_fault_expander_input_cache()
    {
        s_fault_expander_input_cache_valid = false;
        s_fault_expander_input_cache = kFaultExpanderInactiveInputs;
    }

    bool write_fault_expander_register(uint8_t reg, uint8_t value)
    {
        Wire.beginTransmission(config::pins::fault_expander::I2C_ADDRESS);
        Wire.write(reg);
        Wire.write(value);
        return Wire.endTransmission() == 0;
    }

    bool read_fault_expander_register(uint8_t reg, uint8_t& value)
    {
        Wire.beginTransmission(config::pins::fault_expander::I2C_ADDRESS);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0)
        {
            return false;
        }

        const uint8_t bytes_read = Wire.requestFrom(
            config::pins::fault_expander::I2C_ADDRESS,
            static_cast<uint8_t>(1));
        if (bytes_read != 1 || Wire.available() < 1)
        {
            return false;
        }

        value = Wire.read();
        return true;
    }

    bool probe_fault_expander_connection()
    {
        Wire.beginTransmission(config::pins::fault_expander::I2C_ADDRESS);
        return Wire.endTransmission() == 0;
    }

    bool refresh_fault_expander_input_cache()
    {
        uint8_t inputs = kFaultExpanderInactiveInputs;
        if (!read_fault_expander_register(kTca6408aInputPortRegister, inputs))
        {
            invalidate_fault_expander_input_cache();
            return false;
        }

        s_fault_expander_input_cache = inputs;
        s_fault_expander_input_cache_valid = true;
        return true;
    }

    void setup_fault_expander()
    {
#if defined(POPUP_CONTROLLER_BOARD_REV_D)
        s_fault_expander_connected = probe_fault_expander_connection();
        invalidate_fault_expander_input_cache();

        LOG(
            "Fault TCA6408A expander at 0x%02X: %s.",
            config::pins::fault_expander::I2C_ADDRESS,
            s_fault_expander_connected ? "present" : "not present");

        if (!s_fault_expander_connected)
        {
            return;
        }

        if (!write_fault_expander_register(kTca6408aPolarityInversionRegister, 0x00))
        {
            s_fault_expander_connected = false;
            LOG("TCA6408A polarity setup failed.");
            return;
        }

        if (!write_fault_expander_register(kTca6408aConfigurationRegister, 0xFF))
        {
            s_fault_expander_connected = false;
            LOG("TCA6408A input configuration failed.");
            return;
        }

        pinMode(config::pins::fault_expander::INTERRUPT_PIN, INPUT_PULLUP);
        (void)refresh_fault_expander_input_cache();
#else
        s_fault_expander_connected = false;
        invalidate_fault_expander_input_cache();
#endif
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
    setup_fault_expander();
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
        if (!probe_current_external_expander_connection())
        {
            latch_external_expander_disconnect();
            return;
        }

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

bool is_fault_expander_connected()
{
    return s_fault_expander_connected;
}

bool read_fault_expander_pin(IoExpanderPin pin)
{
    if (!s_fault_expander_connected)
    {
        return true;
    }

    if (!refresh_fault_expander_input_cache())
    {
        s_fault_expander_connected = false;
        LOG("Fault TCA6408A input read failed. Fault-expander inputs disabled.");
        return true;
    }

    const uint8_t bit = static_cast<uint8_t>(pin);
    return (s_fault_expander_input_cache & (1u << bit)) != 0u;
}
