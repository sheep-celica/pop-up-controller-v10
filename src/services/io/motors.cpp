#include "services/io/motors.h"

#include "config.h"
#include "services/logging/logging.h"

#if defined(POPUP_CONTROLLER_BOARD_REV_D)
namespace {
    DRV8243::Config make_motor_config(
        gpio_num_t control_pin,
        gpio_num_t coast_disable_pin,
        gpio_num_t current_pin,
        uint8_t ledc_channel)
    {
        return {
            control_pin,
            coast_disable_pin,
            current_pin,
            config::pins::motor_driver::SHARED_SLEEP_PIN,
            ledc_channel,
            config::motors::drv8243::PWM_FREQUENCY_HZ,
            config::motors::drv8243::PWM_RESOLUTION_BITS,
            config::motors::drv8243::NSLEEP_RESET_PULSE_US,
            config::motors::drv8243::DRIVER_READY_DELAY_MS,
            config::motors::drv8243::ADC_REFERENCE_V,
            config::motors::drv8243::ADC_MAX_RAW,
            config::motors::drv8243::IPROPI_SCALING_A_PER_A,
            config::motors::drv8243::IPROPI_TO_GND_OHMS,
            config::motors::drv8243::IPROPI_TO_ADC_OHMS,
            config::motors::drv8243::ADC_TO_GND_OHMS,
            config::motors::drv8243::SAFE_START_ENABLED,
            config::motors::drv8243::SAFE_START_DUTY_PERCENT,
            config::motors::drv8243::SAFE_START_DURATION_MS,
            config::motors::drv8243::STALL_CURRENT_A,
            config::motors::drv8243::STALL_DURATION_MS,
            config::motors::drv8243::STALL_STARTUP_BLANKING_MS,
        };
    }
}

DRV8243 RH_DRV8243_MOTOR(make_motor_config(
    config::pins::RH_MOTOR_ON_PIN,       // DRV8243 ON/BRAKE control pin.
    config::pins::RH_MOTOR_BRAKE_PIN,    // Revision D coast/disable pin; legacy config name kept for now.
    config::pins::RH_CURRENT,
    config::motors::drv8243::LEDC_CHANNEL_RH));

DRV8243 LH_DRV8243_MOTOR(make_motor_config(
    config::pins::LH_MOTOR_ON_PIN,       // DRV8243 ON/BRAKE control pin.
    config::pins::LH_MOTOR_BRAKE_PIN,    // Revision D coast/disable pin; legacy config name kept for now.
    config::pins::LH_CURRENT,
    config::motors::drv8243::LEDC_CHANNEL_LH));
#else
MotorController RH_MOTOR(
    static_cast<int>(config::pins::RH_MOTOR_ON_PIN),
    config::pop_up::ACTIVE_LOW_DRIVE,
    config::pop_up::braking::DEAD_TIME_MS,
    static_cast<int>(config::pins::RH_MOTOR_BRAKE_PIN),
    config::pop_up::braking::LEDC_CHANNEL_RH,
    config::pop_up::braking::FREQUENCY_HZ,
    config::pop_up::braking::PWM_RESOLUTION_BITS,
    config::pop_up::braking::TARGET_DUTY_CYCLE_RATIO,
    config::pop_up::braking::BRAKING_TIME_US,
    config::pop_up::braking::STEP_PERIOD_US,
    config::pop_up::braking::HOLD_TIME_MS);

MotorController LH_MOTOR(
    static_cast<int>(config::pins::LH_MOTOR_ON_PIN),
    config::pop_up::ACTIVE_LOW_DRIVE,
    config::pop_up::braking::DEAD_TIME_MS,
    static_cast<int>(config::pins::LH_MOTOR_BRAKE_PIN),
    config::pop_up::braking::LEDC_CHANNEL_LH,
    config::pop_up::braking::FREQUENCY_HZ,
    config::pop_up::braking::PWM_RESOLUTION_BITS,
    config::pop_up::braking::TARGET_DUTY_CYCLE_RATIO,
    config::pop_up::braking::BRAKING_TIME_US,
    config::pop_up::braking::STEP_PERIOD_US,
    config::pop_up::braking::HOLD_TIME_MS);
#endif

bool setup_motors()
{
#if defined(POPUP_CONTROLLER_BOARD_REV_D)
    const bool rh_ok = RH_DRV8243_MOTOR.begin();
    const bool lh_ok = LH_DRV8243_MOTOR.begin();
    if (rh_ok && lh_ok) {
        // The shared DRV8243 fault outputs can read active until the driver
        // sees its first nSLEEP wake/reset pulse. Prime both drivers here so
        // startup fault polling reflects real hardware faults instead of the
        // pre-wake state.
        RH_DRV8243_MOTOR.wake_up_impulse();
        LOG("Revision D DRV8243 motor drivers initialized.");
    } else {
        LOG("Revision D DRV8243 motor driver initialization failed. RH=%u LH=%u.", rh_ok ? 1u : 0u, lh_ok ? 1u : 0u);
    }
    return rh_ok && lh_ok;
#else
    const bool rh_ok = RH_MOTOR.begin();
    const bool lh_ok = LH_MOTOR.begin();
    return rh_ok && lh_ok;
#endif
}

void update_motors()
{
#if defined(POPUP_CONTROLLER_BOARD_REV_D)
    const uint32_t now_ms = millis();
    RH_DRV8243_MOTOR.update(now_ms);
    LH_DRV8243_MOTOR.update(now_ms);
#endif
}

void prepare_motors_for_sleep()
{
#if defined(POPUP_CONTROLLER_BOARD_REV_D)
    RH_DRV8243_MOTOR.disable();
    LH_DRV8243_MOTOR.disable();
#else
    RH_MOTOR.set_coast();
    LH_MOTOR.set_coast();
#endif
}
