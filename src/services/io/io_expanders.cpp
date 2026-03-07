#include "services/io/io_expanders.h"
#include "config.h"
#include "services/logging/logging.h"

namespace {
    bool try_begin_external_expander_at_address(uint8_t address)
    {
        return remote_pcf.setAddress(address) && remote_pcf.begin();
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

