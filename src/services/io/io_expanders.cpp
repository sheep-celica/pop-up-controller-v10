#include "services/io/io_expanders.h"
#include "config.h"
#include "services/logging/logging.h"

#include <Preferences.h>

namespace {
    constexpr const char* kIoConfigurationNamespace = "io_cfg";
    constexpr const char* kExternalExpanderAddressKey = "ext_i2c";

    Preferences s_io_configuration_preferences;
    bool s_io_configuration_preferences_initialized = false;
    bool s_external_expander_address_cached = false;
    uint8_t s_external_expander_i2c_address = config::pins::external_expander::DEFAULT_I2C_ADDRESS;

    bool is_supported_external_expander_address(uint8_t address)
    {
        return ((address >= 0x20) && (address <= 0x27)) ||
               ((address >= 0x38) && (address <= 0x3F));
    }

    void ensure_io_configuration_preferences()
    {
        if (!s_io_configuration_preferences_initialized)
        {
            s_io_configuration_preferences.begin(kIoConfigurationNamespace, false);
            s_io_configuration_preferences_initialized = true;
        }

        if (!s_external_expander_address_cached)
        {
            if (s_io_configuration_preferences.isKey(kExternalExpanderAddressKey)) {
                s_external_expander_i2c_address = s_io_configuration_preferences.getUChar(
                    kExternalExpanderAddressKey,
                    config::pins::external_expander::DEFAULT_I2C_ADDRESS);
            } else {
                s_external_expander_i2c_address = config::pins::external_expander::DEFAULT_I2C_ADDRESS;
            }

            if (!is_supported_external_expander_address(s_external_expander_i2c_address)) {
                s_external_expander_i2c_address = config::pins::external_expander::DEFAULT_I2C_ADDRESS;
            }

            s_external_expander_address_cached = true;
        }
    }

    bool try_begin_external_expander_at_address(uint8_t address)
    {
        const uint8_t previous_address = remote_pcf.getAddress();
        const bool address_changed = previous_address != address;

        if (address_changed)
        {
            if (!remote_pcf.setAddress(address))
            {
                (void)remote_pcf.setAddress(previous_address);
                return false;
            }
        }

        if (remote_pcf.begin()) {
            return true;
        }

        if (address_changed)
        {
            (void)remote_pcf.setAddress(previous_address);
            (void)remote_pcf.begin();
        }

        return false;
    }
}


// Initiate the helper classes
PCF8574 remote_pcf(config::pins::external_expander::DEFAULT_I2C_ADDRESS);
ADS7138 internal_ads(config::pins::internal_expander::I2C_ADDRESS);

bool is_valid_external_expander_i2c_address(uint8_t address)
{
    return is_supported_external_expander_address(address);
}

uint8_t get_external_expander_i2c_address()
{
    ensure_io_configuration_preferences();
    return s_external_expander_i2c_address;
}

bool set_external_expander_i2c_address(uint8_t address)
{
    if (!is_supported_external_expander_address(address)) {
        return false;
    }

    ensure_io_configuration_preferences();

    const uint8_t previous_address = s_external_expander_i2c_address;

    if (!try_begin_external_expander_at_address(address)) {
        return false;
    }

    const size_t bytes_written = s_io_configuration_preferences.putUChar(
        kExternalExpanderAddressKey,
        address);

    if (bytes_written == sizeof(uint8_t))
    {
        s_external_expander_i2c_address = address;
        s_external_expander_address_cached = true;
        return true;
    }

    if (address != previous_address) {
        (void)try_begin_external_expander_at_address(previous_address);
    }

    s_external_expander_i2c_address = remote_pcf.getAddress();
    s_external_expander_address_cached = true;
    return false;
}

void setup_io_expanders()
{
    const uint8_t configured_external_address = get_external_expander_i2c_address();
    if (!try_begin_external_expander_at_address(configured_external_address))
    {
        const uint8_t default_external_address = config::pins::external_expander::DEFAULT_I2C_ADDRESS;
        if (configured_external_address != default_external_address) {
            (void)try_begin_external_expander_at_address(default_external_address);
        }
    }

    s_external_expander_i2c_address = remote_pcf.getAddress();
    LOG("External expander I2C address: 0x%02X", s_external_expander_i2c_address);

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

