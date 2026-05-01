#include "verification/motor_current_test.h"

#include <Arduino.h>

#include "config.h"
#include "services/logging/logging.h"

namespace {
    constexpr uint8_t kSharedSleepPin = 14;
    constexpr uint32_t kNsleepResetPulseUs = 30;
    constexpr uint32_t kDriverReadyDelayMs = 2;
    constexpr uint32_t kPreRunSampleMs = 200;
    constexpr uint32_t kMotorRunMs = 1000;
    constexpr uint32_t kPostRunSampleMs = 200;
    constexpr uint32_t kSampleDelayMs = 1;
    constexpr uint32_t kPwmFrequencyHz = 1000;
    constexpr uint8_t kPwmResolutionBits = 8;
    constexpr uint8_t kRhPwmChannel = 2;
    constexpr uint32_t kMaxPwmDuty = (1u << kPwmResolutionBits) - 1u;
    constexpr float kAdcReferenceV = 3.3f;
    constexpr float kAdcMaxRaw = 4095.0f;
    constexpr float kIpropiScalingAperA = 3070.0f; // DRV8243HQRXYRQ1 / RXY package.
    constexpr float kIpropiToGndOhms = 1000.0f;
    constexpr float kIpropiToAdcOhms = 22000.0f;
    constexpr float kAdcToGndOhms = 22000.0f;
    constexpr float kAdcDividerRatio = kAdcToGndOhms / (kIpropiToAdcOhms + kAdcToGndOhms);
    constexpr float kEffectiveIpropiOhms =
        1.0f / ((1.0f / kIpropiToGndOhms) + (1.0f / (kIpropiToAdcOhms + kAdcToGndOhms)));

    uint8_t gpio_to_pin(gpio_num_t pin)
    {
        return static_cast<uint8_t>(pin);
    }

    void set_rh_motor_outputs_safe()
    {
        digitalWrite(config::pins::RH_MOTOR_ON_PIN, LOW);
        digitalWrite(config::pins::RH_MOTOR_BRAKE_PIN, HIGH);
    }

    void configure_rh_motor_current_test_pins()
    {
        set_rh_motor_outputs_safe();
        pinMode(config::pins::RH_MOTOR_ON_PIN, OUTPUT);
        pinMode(config::pins::RH_MOTOR_BRAKE_PIN, OUTPUT);
        pinMode(config::pins::RH_CURRENT, INPUT);

        ledcSetup(kRhPwmChannel, kPwmFrequencyHz, kPwmResolutionBits);
        ledcAttachPin(gpio_to_pin(config::pins::RH_MOTOR_ON_PIN), kRhPwmChannel);
        ledcWrite(kRhPwmChannel, 0);
    }

    void wake_motor_drivers()
    {
        digitalWrite(kSharedSleepPin, LOW);
        pinMode(kSharedSleepPin, OUTPUT);
        delay(1);

        LOG("Waking DRV8243H-Q1 motor drivers for current verification.");
        digitalWrite(kSharedSleepPin, HIGH);
        delay(kDriverReadyDelayMs);

        noInterrupts();
        digitalWrite(kSharedSleepPin, LOW);
        delayMicroseconds(kNsleepResetPulseUs);
        digitalWrite(kSharedSleepPin, HIGH);
        interrupts();

        delay(kDriverReadyDelayMs);
        LOG("DRV8243H-Q1 nSLEEP reset pulse complete (%lu us).", static_cast<unsigned long>(kNsleepResetPulseUs));
    }

    void run_rh_motor_full_speed()
    {
        digitalWrite(config::pins::RH_MOTOR_ON_PIN, LOW);
        digitalWrite(config::pins::RH_MOTOR_BRAKE_PIN, LOW);
        delay(1);
        ledcWrite(kRhPwmChannel, kMaxPwmDuty);
    }

    void brake_rh_motor()
    {
        ledcWrite(kRhPwmChannel, 0);
        digitalWrite(config::pins::RH_MOTOR_BRAKE_PIN, LOW);
    }

    const char* phase_name(uint32_t elapsed_ms)
    {
        if (elapsed_ms < kPreRunSampleMs) {
            return "pre";
        }

        if (elapsed_ms < kPreRunSampleMs + kMotorRunMs) {
            return "run";
        }

        return "post";
    }

    float raw_adc_to_adc_voltage(uint16_t raw_adc)
    {
        return (static_cast<float>(raw_adc) * kAdcReferenceV) / kAdcMaxRaw;
    }

    float adc_voltage_to_ipropi_voltage(float adc_voltage)
    {
        return adc_voltage / kAdcDividerRatio;
    }

    float ipropi_voltage_to_motor_current(float ipropi_voltage)
    {
        const float ipropi_current_a = ipropi_voltage / kEffectiveIpropiOhms;
        return ipropi_current_a * kIpropiScalingAperA;
    }
}

void run_motor_current_test()
{
    constexpr uint32_t total_sample_ms = kPreRunSampleMs + kMotorRunMs + kPostRunSampleMs;

    LOG("RH motor current verification started.");
    LOG(
        "Sampling RH current for %lu ms before run, %lu ms at 100%% duty, then %lu ms after braking.",
        static_cast<unsigned long>(kPreRunSampleMs),
        static_cast<unsigned long>(kMotorRunMs),
        static_cast<unsigned long>(kPostRunSampleMs));
    LOG(
        "RH current conversion: ADC divider=%.3f, effective RIPROPI=%.1f ohm, AIPROPI=%.0f A/A.",
        kAdcDividerRatio,
        kEffectiveIpropiOhms,
        kIpropiScalingAperA);

    configure_rh_motor_current_test_pins();
    wake_motor_drivers();

    const uint32_t start_ms = millis();
    bool motor_started = false;
    bool motor_stopped = false;

    while (millis() - start_ms < total_sample_ms) {
        const uint32_t elapsed_ms = millis() - start_ms;

        if (!motor_started && elapsed_ms >= kPreRunSampleMs) {
            motor_started = true;
            LOG("RH motor current verification: starting motor at 100%% duty.");
            run_rh_motor_full_speed();
        }

        if (!motor_stopped && elapsed_ms >= kPreRunSampleMs + kMotorRunMs) {
            motor_stopped = true;
            LOG("RH motor current verification: stopping motor and braking.");
            brake_rh_motor();
        }

        const uint16_t raw_adc = analogRead(config::pins::RH_CURRENT);
        const float adc_voltage = raw_adc_to_adc_voltage(raw_adc);
        const float ipropi_voltage = adc_voltage_to_ipropi_voltage(adc_voltage);
        const float motor_current_a = ipropi_voltage_to_motor_current(ipropi_voltage);

        LOG(
            "RH motor current sample: t=%lu ms phase=%s raw=%u adc=%.3f V ipropi=%.3f V current=%.2f A.",
            static_cast<unsigned long>(elapsed_ms),
            phase_name(elapsed_ms),
            raw_adc,
            adc_voltage,
            ipropi_voltage,
            motor_current_a);

        delay(kSampleDelayMs);
    }

    brake_rh_motor();
    LOG("RH motor current verification complete. Motor stopped/braking.");
}
