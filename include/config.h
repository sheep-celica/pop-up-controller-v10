#pragma once

#include <cstdint>
#include <Arduino.h>
#include "services/io/types/io_expander_pin.h"


namespace config 
{
    // --------- Pop-up configuration ---------- 
    namespace pop_up
    {
        constexpr uint32_t TIMEOUT_MS                       = 2500;
        constexpr uint32_t SENSING_DELAY_US                 = 100;
        constexpr bool     ACTIVE_LOW_DRIVE                 = false;
        constexpr uint32_t DELAY_TO_GO_DOWN_MS              = 200;
        constexpr uint32_t DELAY_TO_GO_UP_MS                = 100;

        namespace braking
        {
            constexpr uint32_t FREQUENCY_HZ                 = 20'000;
            constexpr uint8_t  LEDC_CHANNEL_RH              = 0;
            constexpr uint8_t  LEDC_CHANNEL_LH              = 1;
            constexpr uint8_t  PWM_RESOLUTION_BITS          = 10;
            constexpr float    TARGET_DUTY_CYCLE_PERCENT    = 0.70f;
            constexpr uint32_t BRAKING_TIME_US              = 5'000;
            constexpr uint32_t STEP_PERIOD_US               = 250;
            constexpr uint32_t HOLD_TIME_MS                 = 5'000;
            constexpr uint32_t DEAD_TIME_MS                 = 10;
        }
    }

    namespace utilities
    {
        constexpr uint8_t       STLM75_ADDRESS                  = 0x48;
        constexpr const char*   ERROR_LOG_NAMESPACE             = "error_log";
        constexpr const char*   STATISTICAL_LOG_NAMESPACE       = "statistics";
        constexpr const char*   CALIBRATION_NAMESPACE           = "calibrations";
        constexpr float         BATTERY_DIVIDER_SCALE           = 6.0f;
        
        namespace calibration_keys
        {
            constexpr const char* BAT_VOLTAGE_CONSTANT_A        = "bat_v_a";
            constexpr const char* BAT_VOLTAGE_CONSTANT_B        = "bat_v_b";

        }
    }
    
    // ---------- GPIO Pins ----------
    namespace pins 
    {
        // Pop-up control pins
        constexpr gpio_num_t RH_SENSE_PIN                       = GPIO_NUM_4;
        constexpr gpio_num_t RH_MOTOR_ON_PIN                    = GPIO_NUM_23;
        constexpr gpio_num_t RH_MOTOR_BRAKE_PIN                 = GPIO_NUM_18;
        constexpr gpio_num_t RH_CURRENT                         = GPIO_NUM_39;
        constexpr gpio_num_t LH_SENSE_PIN                       = GPIO_NUM_2;
        constexpr gpio_num_t LH_MOTOR_ON_PIN                    = GPIO_NUM_19;
        constexpr gpio_num_t LH_MOTOR_BRAKE_PIN                 = GPIO_NUM_17;
        constexpr gpio_num_t UP_INPUT_PIN                       = GPIO_NUM_16;
        constexpr gpio_num_t DOWN_INPUT_PIN                     = GPIO_NUM_15;

        // Analogs
        constexpr gpio_num_t SLEEPY_EYE_KNOB_PIN                = GPIO_NUM_34;

        // Buttons
        constexpr gpio_num_t SLEEPY_EYE_BUTTON_PIN              = GPIO_NUM_35;
        constexpr gpio_num_t RH_BUTTON_PIN                      = GPIO_NUM_25;
        constexpr gpio_num_t LH_BUTTON_PIN                      = GPIO_NUM_26;
        constexpr gpio_num_t BH_BUTTON_PIN                      = GPIO_NUM_27;
        constexpr gpio_num_t EXTRA_BUTTON_PIN                   = GPIO_NUM_14;

        // Light-switch pins
        constexpr gpio_num_t LIGHT_SWITCH_UP_PIN                = GPIO_NUM_32;
        constexpr gpio_num_t LIGHT_SWITCH_HOLD_PIN              = GPIO_NUM_33;

        // Illumination
        constexpr gpio_num_t ILLUMINATION_ON_PIN                = GPIO_NUM_12;
        constexpr gpio_num_t POWER_ON_PIN                       = GPIO_NUM_13;


        // SDA/SCL pins
        namespace i2c 
        {
            constexpr gpio_num_t SDA                            = GPIO_NUM_21;
            constexpr gpio_num_t SCL                            = GPIO_NUM_22;
        }

        namespace internal_expander 
        {   
            constexpr uint8_t       I2C_ADDRESS                 = 0x10;
            constexpr IoExpanderPin BATTERY_VOLTAGE_PIN         = IoExpanderPin::PIN_0;  // Analog
            constexpr IoExpanderPin SLEEPY_EYE_LED_PIN          = IoExpanderPin::PIN_1;  // LED
            constexpr IoExpanderPin LED_ADJUST_POT_PIN          = IoExpanderPin::PIN_2;  // Analog
            constexpr IoExpanderPin POP_UP_OFFSET_POT_PIN       = IoExpanderPin::PIN_3;  // Analog
            constexpr IoExpanderPin DEBUG_BUTTON_PIN            = IoExpanderPin::PIN_4;  // Digital input. Button  
            constexpr IoExpanderPin INPUT_LED_PIN               = IoExpanderPin::PIN_5;  // LED
            constexpr IoExpanderPin ERROR_LED_PIN               = IoExpanderPin::PIN_6;  // LED
            constexpr IoExpanderPin STATUS_LED_PIN              = IoExpanderPin::PIN_7;  // LED  
        }

        namespace external_expander
        {
            constexpr uint8_t       I2C_ADDRESS                 = 0x24;
            constexpr IoExpanderPin REMOTE_INPUT_0              = IoExpanderPin::PIN_0;
            constexpr IoExpanderPin REMOTE_INPUT_1              = IoExpanderPin::PIN_1;
            constexpr IoExpanderPin REMOTE_INPUT_2              = IoExpanderPin::PIN_2;
            constexpr IoExpanderPin REMOTE_INPUT_3              = IoExpanderPin::PIN_3;
        }

        // Illumination PWM configuration
        namespace illumination
        {
            constexpr uint32_t FREQUENCY_HZ                     = 1000; // default PWM frequency for illumination
            constexpr uint8_t  PWM_RESOLUTION_BITS              = 8;   // 8-bit resolution
        }

        namespace power
        {
            constexpr uint32_t IDLE_TIME_TO_POWER_OFF_S         = 1000; // Seconds of idle time before powering off
        }
    }
}
