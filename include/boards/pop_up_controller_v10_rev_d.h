#pragma once

#include <Arduino.h>
#include <cstdint>

#include "services/io/types/io_expander_pin.h"

namespace config
{
    namespace board
    {
        constexpr const char* ID = "pop-up-controller-v10-rev-d";
        constexpr const char* DISPLAY_NAME = "Pop-up Controller V10 Revision D";
        constexpr const char* HARDWARE_REVISION = "Revision D";
    }

    namespace features
    {
        // Toggle these on as Revision D hardware support lands in shared code.
        constexpr bool HAS_AUX_OUTPUT = false;
        constexpr bool HAS_AUX_INPUT = false;
        constexpr bool HAS_ONBOARD_REMOTE_RECEIVER = false;
    }

    namespace pins
    {
        // Revision D scaffold:
        // Start from the known-good Revision C map so this build stays valid,
        // then replace each assignment with the new board's actual wiring.

        // Pop-up control pins
        constexpr gpio_num_t RH_SENSE_PIN = GPIO_NUM_4;
        constexpr gpio_num_t RH_MOTOR_ON_PIN = GPIO_NUM_23;
        constexpr gpio_num_t RH_MOTOR_BRAKE_PIN = GPIO_NUM_18;
        constexpr gpio_num_t RH_CURRENT = GPIO_NUM_39;
        constexpr gpio_num_t LH_SENSE_PIN = GPIO_NUM_2;
        constexpr gpio_num_t LH_MOTOR_ON_PIN = GPIO_NUM_19;
        constexpr gpio_num_t LH_MOTOR_BRAKE_PIN = GPIO_NUM_17;
        constexpr gpio_num_t UP_INPUT_PIN = GPIO_NUM_16;
        constexpr gpio_num_t DOWN_INPUT_PIN = GPIO_NUM_15;

        // Analogs
        constexpr gpio_num_t SLEEPY_EYE_KNOB_PIN = GPIO_NUM_34;

        // Buttons
        constexpr gpio_num_t SLEEPY_EYE_BUTTON_PIN = GPIO_NUM_35;
        constexpr gpio_num_t RH_BUTTON_PIN = GPIO_NUM_26;
        constexpr gpio_num_t LH_BUTTON_PIN = GPIO_NUM_25;
        constexpr gpio_num_t BH_BUTTON_PIN = GPIO_NUM_NC;         // TODO: Revision D - IO Expander now
        constexpr gpio_num_t TOGGLE_BUTTON_PIN = GPIO_NUM_NC;     // TODO: Revision D - IO Expander now

        // Light-switch pins
        constexpr gpio_num_t LIGHT_SWITCH_UP_PIN = GPIO_NUM_32;
        constexpr gpio_num_t LIGHT_SWITCH_HOLD_PIN = GPIO_NUM_33;

        // Power pins
        constexpr gpio_num_t ILLUMINATION_ON_PIN = GPIO_NUM_12;
        constexpr gpio_num_t POWER_ON_PIN = GPIO_NUM_13;          // TODO: Revision D - WAKE UP. not pwr on

        namespace i2c
        {
            constexpr uint32_t FREQUENCY_HZ = 100'000;
            constexpr uint16_t TIMEOUT_MS = 10;
            constexpr gpio_num_t SDA = GPIO_NUM_21;
            constexpr gpio_num_t SCL = GPIO_NUM_22;
        }

        namespace internal_expander
        {
            constexpr uint8_t I2C_ADDRESS = 0x10; // TODO: Revision D if changed
            constexpr IoExpanderPin BATTERY_VOLTAGE_PIN = IoExpanderPin::PIN_0;
            constexpr IoExpanderPin SLEEPY_EYE_LED_PIN = IoExpanderPin::PIN_1;
            constexpr IoExpanderPin POP_UP_OFFSET_POT_PIN = IoExpanderPin::PIN_2; // TODO: Revision D - UNASSIGNED NOW
            constexpr IoExpanderPin LED_ADJUST_POT_PIN = IoExpanderPin::PIN_2;
            constexpr IoExpanderPin DEBUG_BUTTON_PIN = IoExpanderPin::PIN_4;
            constexpr IoExpanderPin INPUT_LED_PIN = IoExpanderPin::PIN_5;
            constexpr IoExpanderPin ERROR_LED_PIN = IoExpanderPin::PIN_6;
            constexpr IoExpanderPin STATUS_LED_PIN = IoExpanderPin::PIN_7;
        }

        namespace external_expander
        {
            constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x3C;          // TODO: Revision D if changed
            constexpr uint8_t FALLBACK_I2C_ADDRESS = 0x24;         // TODO: Revision D if changed
            constexpr uint32_t RUNTIME_PROBE_INTERVAL_MS = 100;
            constexpr IoExpanderPin REMOTE_INPUT_0 = IoExpanderPin::PIN_3;
            constexpr IoExpanderPin REMOTE_INPUT_1 = IoExpanderPin::PIN_2;
            constexpr IoExpanderPin REMOTE_INPUT_2 = IoExpanderPin::PIN_1;
            constexpr IoExpanderPin REMOTE_INPUT_3 = IoExpanderPin::PIN_0;
        }

        namespace illumination
        {
            constexpr uint32_t FREQUENCY_HZ = 1000;
            constexpr uint8_t PWM_RESOLUTION_BITS = 8;
            constexpr uint8_t LEDC_CHANNEL_ILLUM = 8;
            constexpr float GAMMA = 2.20f;
            constexpr uint32_t RAMP_TIME_MS = 2000;
            constexpr uint32_t POT_REFRESH_MS_IDLE = 50;
            constexpr uint8_t POT_MIN_DUTY_DELTA = 2;
        }

        namespace power
        {
            constexpr uint32_t IDLE_TIME_TO_POWER_OFF_S = 86400;
            constexpr uint32_t IDLE_COUNTDOWN_LOG_STEP_S = 30;
        }
    }
}
