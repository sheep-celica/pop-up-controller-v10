#pragma once

#include <Arduino.h>
#include <cstdint>

#include "services/io/types/io_expander_pin.h"

#define POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER 1
#define POPUP_CONTROLLER_BOARD_USES_FAULT_EXPANDER 1

namespace config
{
    namespace board
    {
        constexpr const char* ID = "pop-up-controller-v10-rev-e-esp32-s3";
        constexpr const char* DISPLAY_NAME = "Pop-up Controller V10 Revision E ESP32-S3";
        constexpr const char* HARDWARE_REVISION = "Revision E ESP32-S3";
    }

    namespace features
    {
        constexpr bool HAS_AUX_OUTPUT = false;
        constexpr bool HAS_AUX_INPUT = false;
        constexpr bool HAS_ONBOARD_REMOTE_RECEIVER = false;
        constexpr bool HAS_POWER_LATCH = false;
        constexpr bool HAS_DEEP_SLEEP_WAKE = true;
        constexpr bool HAS_EXTERNAL_REMOTE_EXPANDER = true;
        constexpr bool HAS_DRV8243_MOTOR_DRIVER = true;
        constexpr bool HAS_FAULT_EXPANDER = true;
        constexpr bool HAS_SINGLE_TEMPERATURE_SENSOR = false;
        constexpr bool HAS_DUAL_TEMPERATURE_SENSORS = true;
        constexpr bool HAS_RH_POP_UP_OFFSET_POT = true;
    }

    namespace hardware
    {
        constexpr bool MAIN_DIGITAL_LEDS_ACTIVE_LOW = true;

        namespace battery_voltage
        {
            constexpr float DIVIDER_TOP_OHMS = 100'000.0f;
            constexpr float DIVIDER_BOTTOM_OHMS = 10'000.0f;
            constexpr float DIVIDER_SCALE = (DIVIDER_TOP_OHMS + DIVIDER_BOTTOM_OHMS) / DIVIDER_BOTTOM_OHMS;
        }

        namespace temperature
        {
            enum class SensorDriver : uint8_t
            {
                None,
                Lm75Compatible,
                Tmp112,
            };

            constexpr bool HAS_AMBIENT_SENSOR = true;
            constexpr bool HAS_HOTSPOT_SENSOR = true;
            constexpr uint8_t AMBIENT_SENSOR_ADDRESS = 0x48;
            constexpr uint8_t HOTSPOT_SENSOR_ADDRESS = 0x49;
            constexpr SensorDriver AMBIENT_SENSOR_DRIVER = SensorDriver::Tmp112;
            constexpr SensorDriver HOTSPOT_SENSOR_DRIVER = SensorDriver::Tmp112;
        }
    }

    namespace pins
    {
        // GPIO19 and GPIO20 are reserved for native USB D-/D+.
        constexpr gpio_num_t RH_SENSE_PIN = GPIO_NUM_21;
        constexpr gpio_num_t RH_MOTOR_ON_PIN = GPIO_NUM_12;
        constexpr gpio_num_t RH_MOTOR_BRAKE_PIN = GPIO_NUM_35;
        constexpr gpio_num_t RH_CURRENT = GPIO_NUM_5;
        constexpr gpio_num_t LH_SENSE_PIN = GPIO_NUM_14;
        constexpr gpio_num_t LH_MOTOR_ON_PIN = GPIO_NUM_36;
        constexpr gpio_num_t LH_MOTOR_BRAKE_PIN = GPIO_NUM_48;
        constexpr gpio_num_t LH_CURRENT = GPIO_NUM_4;
        constexpr gpio_num_t UP_INPUT_PIN = GPIO_NUM_47;
        constexpr gpio_num_t DOWN_INPUT_PIN = GPIO_NUM_13;
        constexpr bool POSITION_INPUT_ACTIVE_LOW = true;

        // Analogs
        constexpr gpio_num_t SLEEPY_EYE_KNOB_PIN = GPIO_NUM_6;

        // Buttons
        namespace buttons
        {
            constexpr gpio_num_t SLEEPY_EYE_BUTTON_PIN = GPIO_NUM_7;
            constexpr gpio_num_t RH_BUTTON_PIN = GPIO_NUM_8;
            constexpr gpio_num_t LH_BUTTON_PIN = GPIO_NUM_17;
            constexpr gpio_num_t BH_BUTTON_PIN = GPIO_NUM_NC;
            constexpr gpio_num_t TOGGLE_BUTTON_PIN = GPIO_NUM_NC;
        }

        // Light-switch pins
        constexpr gpio_num_t LIGHT_SWITCH_UP_PIN = GPIO_NUM_15;
        constexpr gpio_num_t LIGHT_SWITCH_HOLD_PIN = GPIO_NUM_16;

        // Power and sleep/wake pins
        constexpr gpio_num_t ILLUMINATION_ON_PIN = GPIO_NUM_11;

        namespace power
        {
            constexpr gpio_num_t POWER_LATCH_PIN = GPIO_NUM_NC;
            constexpr gpio_num_t DEEP_SLEEP_WAKE_PIN = GPIO_NUM_18;
            constexpr uint32_t IDLE_TIME_TO_POWER_OFF_S = 30;
            constexpr uint32_t IDLE_COUNTDOWN_LOG_STEP_S = 30;
        }

        namespace motor_driver
        {
            constexpr gpio_num_t SHARED_SLEEP_PIN = GPIO_NUM_10;
        }

        namespace i2c
        {
            constexpr uint32_t FREQUENCY_HZ = 100'000;
            constexpr uint16_t TIMEOUT_MS = 10;
            constexpr gpio_num_t SDA = GPIO_NUM_37;
            constexpr gpio_num_t SCL = GPIO_NUM_38;
        }

        namespace internal_expander
        {
            constexpr uint8_t I2C_ADDRESS = 0x10;
            constexpr IoExpanderPin BATTERY_VOLTAGE_PIN = IoExpanderPin::PIN_0;
            constexpr IoExpanderPin SLEEPY_EYE_LED_PIN = IoExpanderPin::PIN_1;
            constexpr IoExpanderPin POP_UP_OFFSET_POT_PIN = IoExpanderPin::PIN_3;
            constexpr IoExpanderPin LED_ADJUST_POT_PIN = IoExpanderPin::PIN_2;
            constexpr IoExpanderPin DEBUG_BUTTON_PIN = IoExpanderPin::PIN_4;
            constexpr IoExpanderPin INPUT_LED_PIN = IoExpanderPin::PIN_5;
            constexpr IoExpanderPin ERROR_LED_PIN = IoExpanderPin::PIN_6;
            constexpr IoExpanderPin STATUS_LED_PIN = IoExpanderPin::PIN_7;
        }

        namespace external_expander
        {
            constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x3C;
            constexpr uint8_t FALLBACK_I2C_ADDRESS = 0x24;
            constexpr uint32_t RUNTIME_PROBE_INTERVAL_MS = 100;
            constexpr IoExpanderPin REMOTE_INPUT_0 = IoExpanderPin::PIN_3;
            constexpr IoExpanderPin REMOTE_INPUT_1 = IoExpanderPin::PIN_2;
            constexpr IoExpanderPin REMOTE_INPUT_2 = IoExpanderPin::PIN_1;
            constexpr IoExpanderPin REMOTE_INPUT_3 = IoExpanderPin::PIN_0;
        }

        namespace fault_expander
        {
            constexpr uint8_t I2C_ADDRESS = 0x21;
            constexpr gpio_num_t INTERRUPT_PIN = GPIO_NUM_9;
            constexpr bool FAULT_INPUT_ACTIVE_LOW = true;
            constexpr IoExpanderPin RH_MOTOR_FAULT_PIN = IoExpanderPin::PIN_0;
            constexpr IoExpanderPin LH_MOTOR_FAULT_PIN = IoExpanderPin::PIN_1;
            constexpr IoExpanderPin RH_SENSE_FAULT_PIN = IoExpanderPin::PIN_2;
            constexpr IoExpanderPin LH_SENSE_FAULT_PIN = IoExpanderPin::PIN_3;
            constexpr IoExpanderPin ILLUMINATION_FAULT_PIN = IoExpanderPin::PIN_5;
            constexpr IoExpanderPin BH_BUTTON_PIN = IoExpanderPin::PIN_6;
            constexpr IoExpanderPin TOGGLE_BUTTON_PIN = IoExpanderPin::PIN_7;
            constexpr uint32_t RUNTIME_POLL_INTERVAL_MS = 100;
        }

        namespace illumination
        {
            constexpr uint32_t FREQUENCY_HZ = 1000;
            constexpr uint8_t PWM_RESOLUTION_BITS = 8;
            constexpr uint8_t LEDC_CHANNEL_ILLUM = 4;
            constexpr float GAMMA = 2.20f;
            constexpr uint32_t RAMP_TIME_MS = 2000;
            constexpr uint32_t POT_REFRESH_MS_IDLE = 50;
            constexpr uint8_t POT_MIN_DUTY_DELTA = 2;
        }
    }
}
