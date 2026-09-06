#pragma once

#include <cstdint>
#include <Arduino.h>
#include "board_config.h"


namespace config 
{
    // --------- Pop-up configuration ---------- 
    namespace pop_up
    {
        constexpr uint32_t TIMEOUT_MS                       = 2500;
        constexpr uint32_t FORCE_POLL_PERIOD_MS             = 5000;
        constexpr uint32_t SENSING_DELAY_US                 = 1000;
        constexpr uint32_t MIN_STATE_PERSIST_MS             = 5;
        constexpr bool     ACTIVE_LOW_DRIVE                 = false;
        constexpr uint32_t DELAY_TO_GO_DOWN_MS              = 200;
        constexpr uint32_t DELAY_TO_GO_UP_MS                = 100;
        constexpr uint32_t RH_POP_UP_OFFSET_RANGE_MS        = 50;   // Allows for adjustment between -50 to 50 ms using the OFFSET potentiomer

        namespace timing_calibration
        {
            constexpr uint16_t    MIN_BATTERY_VOLTAGE_DV    = 110;      // 11.0 V
            constexpr uint16_t    MAX_BATTERY_VOLTAGE_DV    = 150;      // 15.0 V
            constexpr uint16_t    MIN_DOWN_TIME_MS          = 300;
            constexpr uint16_t    MAX_DOWN_TIME_MS          = 1000;
            constexpr uint16_t    DEFAULT_DOWN_TIME_MS      = 600;
            constexpr const char* PREFERENCES_KEY           = "dt_tbl"; // use separate Preferences namespaces per pop-up
        }

        namespace braking
        {
            constexpr uint32_t FREQUENCY_HZ                 = 20'000;
            constexpr uint8_t  LEDC_CHANNEL_RH              = 0;
            constexpr uint8_t  LEDC_CHANNEL_LH              = 1;
            constexpr uint8_t  PWM_RESOLUTION_BITS          = 10;
            constexpr float    TARGET_DUTY_CYCLE_RATIO      = 1.00f;
            constexpr uint32_t BRAKING_TIME_US              = 0;
            constexpr uint32_t STEP_PERIOD_US               = 250;
            constexpr uint32_t HOLD_TIME_MS                 = 3000; //200;
            constexpr uint32_t DEAD_TIME_MS                 = 10;
        }
    }

    namespace utilities
    {
        constexpr const char*   ERROR_LOG_NAMESPACE             = "error_log";
        constexpr const char*   STATISTICAL_LOG_NAMESPACE       = "statistics";
        constexpr const char*   MANUFACTURING_NAMESPACE         = "mfg_data";
        constexpr const char*   CALIBRATION_NAMESPACE           = "calibrations";
        constexpr const char*   FAULT_CONFIGURATION_NAMESPACE   = "fault_cfg";
        constexpr uint8_t       BATTERY_VOLTAGE_AVERAGE_SAMPLES = 8;
        constexpr uint8_t       BATTERY_TIMING_PROFILE_RUNS     = 8;
        constexpr float         BENCH_MODE_MAX_BATTERY_V        = 7.0f;  // Below this, treat supply as USB/bench rather than car battery.

        namespace statistics
        {
            // Total runtime counter is persisted at this interval to reduce flash wear.
            constexpr uint32_t RUNTIME_FLUSH_SECONDS            = 600; // 10 minutes
        }
        
        namespace calibration_keys
        {
            constexpr const char* BAT_VOLTAGE_CONSTANT_A        = "bat_v_a";
            constexpr const char* BAT_VOLTAGE_CONSTANT_B        = "bat_v_b";

        }
    }

    namespace motors
    {
        namespace drv8243
        {
            constexpr uint32_t PWM_FREQUENCY_HZ                 = 1000;
            constexpr uint8_t  PWM_RESOLUTION_BITS              = 8;
            constexpr uint8_t  LEDC_CHANNEL_RH                  = 2;
            constexpr uint8_t  LEDC_CHANNEL_LH                  = 3;

            constexpr uint32_t NSLEEP_RESET_PULSE_US            = 30;
            constexpr uint32_t DRIVER_READY_DELAY_MS            = 2;

            constexpr float ADC_REFERENCE_V                     = 3.3f;
            constexpr float ADC_MAX_RAW                         = 4095.0f;
            constexpr float IPROPI_SCALING_A_PER_A              = 3070.0f;
            constexpr float IPROPI_TO_GND_OHMS                  = 1000.0f;
            constexpr float IPROPI_TO_ADC_OHMS                  = 22000.0f;
            constexpr float ADC_TO_GND_OHMS                     = 22000.0f;

            constexpr bool SAFE_START_ENABLED                   = true;
            constexpr float SAFE_START_DUTY_PERCENT             = 100.0f;
            constexpr uint32_t SAFE_START_DURATION_MS           = 200;

            constexpr float STALL_CURRENT_A                     = 5.0f;
            constexpr uint32_t STALL_DURATION_MS                = 200;
            constexpr uint32_t STALL_STARTUP_BLANKING_MS        = 200;
            constexpr bool     STALL_PROTECTION_ENABLED         = true;

            constexpr const char* CURRENT_CALIBRATION_NAMESPACE = "motor_cal";
            constexpr const char* RH_CURRENT_SCALE_KEY         = "rh_scale";
            constexpr const char* RH_CURRENT_OFFSET_KEY        = "rh_off";
            constexpr const char* LH_CURRENT_SCALE_KEY         = "lh_scale";
            constexpr const char* LH_CURRENT_OFFSET_KEY        = "lh_off";
            constexpr uint32_t CALIBRATION_DURATION_MS          = 1500;
            constexpr uint32_t CALIBRATION_SAMPLE_PERIOD_MS     = 100;
            constexpr uint32_t CALIBRATION_SETTLE_MS            = 100;
            constexpr uint32_t CURRENT_TEST_DURATION_MS         = 2000;
            constexpr uint32_t CURRENT_TEST_MAX_DURATION_MS     = 10000;
            constexpr uint32_t CURRENT_TEST_SAMPLE_PERIOD_MS    = 1;
            constexpr uint32_t CURRENT_TEST_REPORT_PERIOD_MS    = 100;
        }
    }
}
