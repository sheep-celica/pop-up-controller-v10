#include "services/pop_up_control.h"
#include "config.h"

// Main Motor and Pop-up classes
MotorController RH_MOTOR(
  static_cast<int>(config::pins::RH_MOTOR_ON_PIN),          // pmos_gate_pin
  config::pop_up::ACTIVE_LOW_DRIVE,                         // if pmos control is active low or active high (active low = true)
  config::pop_up::braking::DEAD_TIME_MS,                    // deadtime_ms
  static_cast<int>(config::pins::RH_MOTOR_BRAKE_PIN),       // nmos_pwm_pin
  config::pop_up::braking::LEDC_CHANNEL_RH,                 // nmos_ledc_channel
  config::pop_up::braking::FREQUENCY_HZ,                    // nmos_pwm_freq_hz
  config::pop_up::braking::PWM_RESOLUTION_BITS,             // nmos_pwm_resolution_bits
  config::pop_up::braking::TARGET_DUTY_CYCLE_PERCENT,       // nmos_target_duty (0..1)
  config::pop_up::braking::BRAKING_TIME_US,                 // nmos_ramp_time_us
  config::pop_up::braking::STEP_PERIOD_US,                  // nmos_step_period_us
  config::pop_up::braking::HOLD_TIME_MS                     // nmos_hold_time_ms
);
MotorController LH_MOTOR(
  static_cast<int>(config::pins::LH_MOTOR_ON_PIN),
  config::pop_up::ACTIVE_LOW_DRIVE,
  config::pop_up::braking::DEAD_TIME_MS,
  static_cast<int>(config::pins::LH_MOTOR_BRAKE_PIN),
  config::pop_up::braking::LEDC_CHANNEL_LH,
  config::pop_up::braking::FREQUENCY_HZ,
  config::pop_up::braking::PWM_RESOLUTION_BITS,
  config::pop_up::braking::TARGET_DUTY_CYCLE_PERCENT,
  config::pop_up::braking::BRAKING_TIME_US,
  config::pop_up::braking::STEP_PERIOD_US,
  config::pop_up::braking::HOLD_TIME_MS
);

PopUp RH_POP_UP(&RH_MOTOR, config::pins::RH_SENSE_PIN);
PopUp LH_POP_UP(&LH_MOTOR, config::pins::LH_SENSE_PIN);


// Public functions
void setup_pop_ups()
/*
To be run once in the beginning to perform initial configuration of pop-ups.
*/
{
  RH_MOTOR.begin();
  LH_MOTOR.begin();
}

void update_pop_ups()
/*
Update the pop-ups if their current target is not IDLE
*/
{
    if (RH_POP_UP.get_target() != PopUpState::IDLE)
    {
        RH_POP_UP.update();
    }

    if (LH_POP_UP.get_target() != PopUpState::IDLE)
    {
        LH_POP_UP.update();
    }
}
