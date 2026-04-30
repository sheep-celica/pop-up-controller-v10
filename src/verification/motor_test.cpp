#include "verification/motor_test.h"

#include <Arduino.h>

#include "services/logging/logging.h"

namespace {
    constexpr uint8_t kRhOnPin = 23;
    constexpr uint8_t kRhCoastPin = 18;
    constexpr uint8_t kRhCurrentPin = 39;

    constexpr uint8_t kLhOnPin = 19;
    constexpr uint8_t kLhCoastPin = 17;
    constexpr uint8_t kLhCurrentPin = 36;

    constexpr uint8_t kSleepPin = 14;

    constexpr uint32_t kNsleepResetPulseUs = 30;
    constexpr uint32_t kDriverReadyDelayMs = 2;
    constexpr uint32_t kMotorRunTimeMs = 1000;
    constexpr uint32_t kPwmFrequencyHz = 1000;
    constexpr uint8_t kPwmResolutionBits = 8;
    constexpr uint8_t kRhPwmChannel = 2;
    constexpr uint8_t kLhPwmChannel = 3;
    constexpr uint8_t kPwmDutyPercent = 60;
    constexpr uint32_t kMaxPwmDuty = (1u << kPwmResolutionBits) - 1u;
    constexpr uint32_t kRunPwmDuty = (kMaxPwmDuty * kPwmDutyPercent) / 100u;

    struct MotorPins {
        const char* name;
        uint8_t on_pin;
        uint8_t coast_pin;
        uint8_t current_pin;
        uint8_t pwm_channel;
    };

    constexpr MotorPins kRhMotor = { "RH", kRhOnPin, kRhCoastPin, kRhCurrentPin, kRhPwmChannel };
    constexpr MotorPins kLhMotor = { "LH", kLhOnPin, kLhCoastPin, kLhCurrentPin, kLhPwmChannel };

    void set_motor_outputs_safe(const MotorPins& motor)
    {
        digitalWrite(motor.on_pin, LOW);
        digitalWrite(motor.coast_pin, HIGH);
    }

    void configure_motor_pins(const MotorPins& motor)
    {
        set_motor_outputs_safe(motor);
        pinMode(motor.on_pin, OUTPUT);
        pinMode(motor.coast_pin, OUTPUT);
        pinMode(motor.current_pin, INPUT);

        ledcSetup(motor.pwm_channel, kPwmFrequencyHz, kPwmResolutionBits);
        ledcAttachPin(motor.on_pin, motor.pwm_channel);
        ledcWrite(motor.pwm_channel, 0);
    }

    void wake_motor_drivers()
    {
        digitalWrite(kSleepPin, LOW);
        pinMode(kSleepPin, OUTPUT);
        delay(1);

        LOG("Waking DRV8243H-Q1 motor drivers.");
        digitalWrite(kSleepPin, HIGH);
        delay(kDriverReadyDelayMs);

        noInterrupts();
        digitalWrite(kSleepPin, LOW);
        delayMicroseconds(kNsleepResetPulseUs);
        digitalWrite(kSleepPin, HIGH);
        interrupts();

        delay(kDriverReadyDelayMs);
        LOG("DRV8243H-Q1 nSLEEP reset pulse complete (%lu us).", static_cast<unsigned long>(kNsleepResetPulseUs));
    }

    void enable_motor_driver(const MotorPins& motor)
    {
        digitalWrite(motor.on_pin, LOW);
        digitalWrite(motor.coast_pin, LOW);
        delay(1);
    }

    void brake_motor_driver(const MotorPins& motor)
    {
        ledcWrite(motor.pwm_channel, 0);
        digitalWrite(motor.coast_pin, LOW);
    }

    void run_motor_for_test(const MotorPins& motor)
    {
        LOG(
            "%s motor current before run: raw=%d.",
            motor.name,
            analogRead(motor.current_pin));

        LOG(
            "%s motor PWM at %u%% for %lu ms.",
            motor.name,
            kPwmDutyPercent,
            static_cast<unsigned long>(kMotorRunTimeMs));
        enable_motor_driver(motor);
        ledcWrite(motor.pwm_channel, kRunPwmDuty);
        delay(kMotorRunTimeMs);

        brake_motor_driver(motor);
        LOG(
            "%s motor stopped/braking. current raw=%d.",
            motor.name,
            analogRead(motor.current_pin));
        delay(500);
    }
}

void setup_motor_test()
{
    LOG("Motor verification started.");

    configure_motor_pins(kRhMotor);
    configure_motor_pins(kLhMotor);
    wake_motor_drivers();

    run_motor_for_test(kRhMotor);
    run_motor_for_test(kLhMotor);

    brake_motor_driver(kRhMotor);
    brake_motor_driver(kLhMotor);
    LOG("Motor verification complete. Both motor drivers are stopped/braking.");
}
