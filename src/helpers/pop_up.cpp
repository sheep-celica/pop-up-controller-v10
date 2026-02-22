#include <Arduino.h>
#include "helpers/pop_up.h"
#include "services/logging/logging.h"
#include "services/io/leds.h"
#include "config.h"


PopUp::PopUp(MotorController* motor_controller, int sensing_pin, PopUpId pop_up_id)
    : 
      // Variable initialization
      pop_up_id(pop_up_id),
      motor_controller(motor_controller),
      sensing_pin(sensing_pin),
      winking(false),
      auto_toggle_target(false),
      sleepy_eye_mode(false),
      sleepy_eye_move_time(-1),
      movement_start_time(-1),
      current_target(PopUpState::IDLE),
      previous_target(PopUpState::IDLE),
      is_moving(false),
      timing_calibration(PopUpTimingCalibration())
{
    // Pin initialization
    pinMode(sensing_pin, OUTPUT);
    digitalWrite(sensing_pin, LOW);
}

void PopUp::set_target(PopUpState target)
{
    if (current_target == PopUpState::TIMEOUT)
    {
        LOG("Pop-up has timed-out! Can not set a new target.");
        return;
    }
    current_target = target;

    if (target != PopUpState::UP && target != PopUpState::DOWN)
    {
        PopUpState current_state = get_state();

        if (current_state == target)
        {
          previous_target = target;
          current_target = PopUpState::IDLE;
          return;
        }

        // The update function will take care of starting the motor if needed
        current_target = target;
    }
}
void PopUp::set_sleepy_eye_mode(bool active)
{
    sleepy_eye_mode = active;
    if (sleepy_eye_mode)
    {
        PopUpState current_state = get_state();

        if (current_state != PopUpState::UP)  // We need to go UP first to have a defined starting point
        {
            current_target = PopUpState::UP;
            return;
        }

        // We are already UP
        sleepy_eye_move_time = _get_sleepy_eye_move_time();
        set_target(PopUpState::IN_BETWEEN);

    }
}

void PopUp::wink_pop_up()
{
  if (current_target == PopUpState::IDLE)
  {
    PopUpState current_state = get_state();
    PopUpState new_target = (current_state == PopUpState::DOWN) ? PopUpState::UP : PopUpState::DOWN;
    
    LOG("PopUp %s: Winking from state %d to %d", name(), static_cast<int>(current_state), static_cast<int>(new_target));
    
    winking = true;
    auto_toggle_target = true;

    set_target(new_target);
  }
}

void PopUp::update()
{
    if (current_target == PopUpState::IDLE || current_target == PopUpState::TIMEOUT)
    {
      return;  // No need to update while IDLE or TIMED OUT
    }

    PopUpState current_state = get_state();

    if (current_state != current_target)  // We have a valid target position which the pop-up has not reached yet.
    {
      if (!is_moving)  // Start motor if not running.
      {
          _start_pop_up();
      }

      int elapsed_time = millis() - movement_start_time;

      if (elapsed_time > config::pop_up::TIMEOUT_MS)  // Disable motor if position is not reached in reasonable time
      {
          _stop_motor(true);
      }

      if (current_target == PopUpState::IN_BETWEEN && elapsed_time >= sleepy_eye_move_time){
          _stop_motor(false);
      }
    }

    if (current_state == current_target)
    {
      if (current_target == PopUpState::UP && sleepy_eye_mode)
      {
        sleepy_eye_move_time = _get_sleepy_eye_move_time();
        current_target = PopUpState::IN_BETWEEN;
        return;
      }
      
      // Only log if motor was actually running (avoid logging spurious matches on startup)
      if (is_moving || movement_start_time > 0)
      {
        LOG("Pop-up reached target in %d ms.", millis()-movement_start_time);
      }

      if (auto_toggle_target)
      {
        auto_toggle_target = false;
        // Toggle to opposite target (UP <-> DOWN)
        PopUpState new_target = (current_target == PopUpState::DOWN) ? PopUpState::UP : PopUpState::DOWN;
        LOG("PopUp %s: Auto-Retarget from %d to %d", name(), static_cast<int>(current_target), static_cast<int>(new_target));
        set_target(new_target);
        return;
      }

      _stop_motor(false);
    }
    

}

PopUpState PopUp::get_state() const
{
    int opposite_sensing_pin = sensing_pin == config::pins::RH_SENSE_PIN ? config::pins::LH_SENSE_PIN : config::pins::RH_SENSE_PIN;

    // Read UP and DOWN signals from the optocouplers
    digitalWrite(opposite_sensing_pin, LOW);
    digitalWrite(sensing_pin, HIGH);
    delayMicroseconds(config::pop_up::SENSING_DELAY_US);
    bool up_state = digitalRead(config::pins::UP_INPUT_PIN);
    bool down_state = digitalRead(config::pins::DOWN_INPUT_PIN);

    // LOG("PopUp %s: up_state=%d, down_state=%d", name(), up_state, down_state);

    if (up_state && !down_state)
    {
        return PopUpState::UP;
    }
    else if (!up_state && down_state)
    {
        return PopUpState::DOWN;
    }
    return PopUpState::IN_BETWEEN;
}

PopUpState PopUp::get_target() const
{
    return current_target;
}

PopUpState PopUp::get_previous_target() const
{
    return previous_target;
}

bool PopUp::get_sleepy_eye_mode() const
{
    return sleepy_eye_mode;
}

bool PopUp::is_winking() const
{
    return winking;
}

const char* PopUp::name() const
{
    switch (pop_up_id)
    {
        case PopUpId::RH: return "RH";
        case PopUpId::LH: return "LH";
        default:          return "?";
    }
}


// Private helpers
void PopUp::_start_pop_up()
{
  motor_controller->set_run(true);
  movement_start_time = millis();
  is_moving = true;
  LOG("PopUp %s: Started motor. Target=%d", name(), static_cast<int>(current_target));
}

void PopUp::_stop_motor(bool timed_out)
{
    // Only log if motor was actually running (avoid logging when movement_start_time is invalid -1)
    if (is_moving && movement_start_time > 0)
    {
        int move_duration = millis() - movement_start_time;
        const char* reason = timed_out ? "TIMEOUT" : "normal";
        LOG("PopUp %s: Stopped motor. Reason=%s, Duration=%d ms", name(), reason, move_duration);
    }
    
    LOG("PopUp %s: Starting to brake!", name());
    motor_controller->set_brake(true);

    if (timed_out)
    {
        set_led_state(LedId::ERROR_LED, true);
    }

    is_moving = false;
    winking = false;
    PopUpState new_target = (timed_out) ? PopUpState::TIMEOUT : PopUpState::IDLE;
    previous_target = current_target;
    current_target = new_target;

    // TODO: If target was DOWN add the new movetime to the calibration table. Make sure to only do this if the PopUp moved from UP.

}

int PopUp::_get_sleepy_eye_move_time()
{
    /*
    TODO:
    Calculate the move time from:
    - Knob position
    - Battery voltage and latest expected value for time to go down for this voltage
    */
    return 200; // TOOD: We need to calculate this
}
