#include "services/pop_up_control/pop_up_control.h"
#include "services/logging/logging.h"
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

PopUp RH_POP_UP(&RH_MOTOR, config::pins::RH_SENSE_PIN, PopUpId::RH);
PopUp LH_POP_UP(&LH_MOTOR, config::pins::LH_SENSE_PIN, PopUpId::LH);


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

void safe_move_pop_up_to(PopUp *pop_up, PopUpState target)
{
  if (pop_up->get_target() == PopUpState::TIMEOUT)
  {
    // Do not move if pop-up is timed out.
    return;
  }

  if (pop_up->get_target() == PopUpState::IDLE && pop_up->get_previous_target() != target)
  {
    // Set new target if pop-up is IDLE and has not reached this target on previous move.
    LOG("Moving %s Pop-up to %s", pop_up->name(), pop_up_state_name(target));
    pop_up->set_target(target);
  }

  if(pop_up->get_target() != PopUpState::IDLE && pop_up->get_target() != target)
  {
    // Set new target if pop-up is not IDLE but trying to reach a target other than the currently specified.
    LOG("Moving %s Pop-up to %s. Target switched from %s", pop_up->name(), pop_up_state_name(target), pop_up_state_name(pop_up->get_target()));
    pop_up->set_target(target);
  }

}
