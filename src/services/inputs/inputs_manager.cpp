#include "services/inputs/inputs_manager.h"
#include "config.h"


void InputManager::add(Input& input)
{
    inputs.push_back(&input);
}

void InputManager::update()
{
    uint32_t now_ms = millis();

    for (auto* input : inputs)
    {
        input->update(now_ms);
    }
}

// Initiate the helper classes
InputManager inputs_manager;
PCF8574 remote_pcf(config::pins::external_expander::I2C_ADDRESS);
ADS7138 internal_ads(config::pins::internal_expander::I2C_ADDRESS);


void setup_io_expanders()
{
    internal_ads.setAnalogInput(    static_cast<uint8_t> (config::pins::internal_expander::BATTERY_VOLTAGE_PIN      ));
    internal_ads.setAnalogInput(    static_cast<uint8_t> (config::pins::internal_expander::LED_ADJUST_POT_PIN       ));
    internal_ads.setAnalogInput(    static_cast<uint8_t> (config::pins::internal_expander::POP_UP_OFFSET_POT_PIN    ));
    internal_ads.setDigitalOutput(  static_cast<uint8_t> (config::pins::internal_expander::INPUT_LED_PIN            ), true);
    internal_ads.setDigitalOutput(  static_cast<uint8_t> (config::pins::internal_expander::ERROR_LED_PIN            ), true);
    internal_ads.setDigitalOutput(  static_cast<uint8_t> (config::pins::internal_expander::STATUS_LED_PIN           ), true);
    internal_ads.setDigitalOutput(  static_cast<uint8_t> (config::pins::internal_expander::SLEEPY_EYE_LED_PIN       ), true);
    internal_ads.setDigitalInput(   static_cast<uint8_t> (config::pins::internal_expander::DEBUG_BUTTON_PIN         ));

    remote_pcf.begin();
    internal_ads.begin();

}

