#include <Arduino.h>
#include "types/pop_up.h"
#include "services/logging.h"
#include "config.h"


PopUp::PopUp(int control_pin, int sensing_pin)
    : 
      // Variable initialization
      control_pin(control_pin),
      sensing_pin(sensing_pin),
      is_winking(false),
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
    pinMode(control_pin, OUTPUT);
    pinMode(sensing_pin, OUTPUT);
    digitalWrite(control_pin, LOW);
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
          current_target == PopUpState::IDLE;
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
    is_winking = true;
    auto_toggle_target = true;
    PopUpState new_target = (previous_target == PopUpState::DOWN) ? PopUpState::UP : PopUpState::DOWN;

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

      if (elapsed_time > config::POP_UP_TIMEOUT_MS)  // Disable motor if position is not reached in reasonable time
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
      LOG("Pop-up reached target in %d ms.", millis()-movement_start_time);
      _stop_motor(false);
    }
    

}

PopUpState PopUp::get_state() const
{
    int opposite_sensing_pin = sensing_pin == config::pins::RH_SENSE_PIN ? config::pins::LH_SENSE_PIN : config::pins::RH_SENSE_PIN;

    // Read UP and DOWN signals from the optocouplers
    digitalWrite(opposite_sensing_pin, LOW);
    digitalWrite(sensing_pin, HIGH);
    delayMicroseconds(config::SENSING_DELAY_US);
    bool up_state = digitalRead(config::pins::UP_INPUT_PIN);
    bool down_state = digitalRead(config::pins::DOWN_INPUT_PIN);

    if (up_state && !down_state) return PopUpState::UP;
    else if (!up_state && down_state) return PopUpState::DOWN;
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

// Private helpers
void PopUp::_start_pop_up()
{
  digitalWrite(control_pin, HIGH);
  movement_start_time = millis();
  is_moving = true;
}

void PopUp::_stop_motor(bool timed_out)
{
    digitalWrite(control_pin, LOW);

    is_moving = false;
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