#include <Arduino.h>
#include "helpers/pop_up.h"
#include "services/logging/logging.h"
#include "services/io/leds.h"
#include "services/logging/statistics_manager.h"
#include "services/inputs/types/sleepy_position_knob.h"
#include "services/utilities/utilities.h"
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
      initialized_(false),
      init_warning_logged_(false),
      timing_calibration(PopUpTimingCalibration())
{
}

void PopUp::begin()
{
    pinMode(sensing_pin, OUTPUT);
    digitalWrite(sensing_pin, LOW);
    initialized_ = true;
    init_warning_logged_ = false;
}

void PopUp::set_target(PopUpState target)
{
    if (!initialized_)
    {
        _log_not_initialized_once();
        return;
    }

    if (current_target == PopUpState::TIMEOUT)
    {
        LOG("Pop-up has timed-out! Can not set a new target.");
        return;
    }

    // The update function will take care of starting the motor if needed.
    current_target = target;
    if (is_moving)
    {
        // Resetting movement start time to prevent unwanted timeout.
        uint32_t moving_time_ms = millis()-movement_start_time;
        statistics_manager.record_pop_up_move_time(pop_up_id, moving_time_ms);

        movement_start_time = millis();
    }

    if ((target == PopUpState::UP || target == PopUpState::DOWN) && !sleepy_eye_mode)
    {
        PopUpState current_state = get_state();

        if (current_state == target)
        {
          LOG("PopUp %s: Target %d already reached. Staying idle.",
              name(),
              static_cast<int>(target));

          if (is_moving)
          {
              LOG(
                  "PopUp %s: Target reached while moving. Stopping motor to avoid runaway.",
                  name());
              _stop_motor(false);
          }

          previous_target = target;
          current_target = PopUpState::IDLE;
          return;
        }
    }
}

void PopUp::reset_timeout()
{
    current_target = PopUpState::IDLE;
    previous_target = PopUpState::IDLE;
    LOG("PopUp %s has been reset from the time-out state.", name());
}

void PopUp::set_sleepy_eye_mode(bool active)
{
    if (!initialized_)
    {
        _log_not_initialized_once();
        return;
    }

    sleepy_eye_mode = active;
    if (sleepy_eye_mode)
    {
        PopUpState current_state = get_state();
        current_target = PopUpState::UP;
    }
}

void PopUp::wink_pop_up()
{
  if (!initialized_)
  {
    _log_not_initialized_once();
    return;
  }

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
    if (!initialized_)
    {
        _log_not_initialized_once();
        return;
    }

    if (current_target == PopUpState::IDLE)
    {
      if (is_moving)
      {
          LOG("PopUp %s: Safety stop triggered (IDLE target while motor marked moving).", name());
          _stop_motor(false);
      }

      return;  // No need to update while IDLE
    }

    if (current_target == PopUpState::TIMEOUT)
    {
      return;  // No need to update while TIMED OUT
    }

    // Handle targeting to Sleepy Eye Position. The sleepy_eye_move time is calculated later in this function or from set_sleepy_eye_mode.
    if (current_target == PopUpState::IN_BETWEEN)
    {
        uint32_t movement_time = millis() - movement_start_time;
        if (movement_time >= sleepy_eye_move_time)
        {
            LOG("PopUp %s reached sleepy eye position of %d ms in %d ms", name(), sleepy_eye_move_time, movement_time);
            _stop_motor(false);
        }
        return;
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
    }
    else
    {
      if (current_target == PopUpState::UP && sleepy_eye_mode)
      {
        LOG("Pop-up %s reached the UP position with scheduled Sleepy Eye mode. Moving to Sleepy Eye Position now.", name());

        sleepy_eye_move_time = _get_sleepy_eye_move_time();
        current_target = PopUpState::IN_BETWEEN;

        if (is_moving)
        {
            uint32_t moving_time_ms = millis()-movement_start_time;
            statistics_manager.record_pop_up_move_time(pop_up_id, moving_time_ms);
            movement_start_time = millis();
            return;
        }

        _start_pop_up();
        return;
      }

      if (previous_target == PopUpState::UP && current_target == PopUpState::DOWN)
      {
        uint32_t time_to_go_down_ms = millis() - movement_start_time;
        float battery_voltage = read_battery_voltage();
        LOG("PopUp %s Reached DOWN position from UP position in %d ms at voltage %.1f V. Saving to PopupTimingCalibration.", name(), time_to_go_down_ms, battery_voltage);
        timing_calibration.add_sample(battery_voltage, time_to_go_down_ms);
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
    if (!initialized_)
    {
        _log_not_initialized_once();
        return PopUpState::IN_BETWEEN;
    }

    static_assert(config::pop_up::SENSING_SAMPLE_COUNT > 0, "SENSING_SAMPLE_COUNT must be >= 1");

    int opposite_sensing_pin = sensing_pin == config::pins::RH_SENSE_PIN ? config::pins::LH_SENSE_PIN : config::pins::RH_SENSE_PIN;

    uint8_t up_votes = 0;
    uint8_t down_votes = 0;
    uint8_t in_between_votes = 0;

    for (uint8_t i = 0; i < config::pop_up::SENSING_SAMPLE_COUNT; ++i)
    {
        // Read UP and DOWN signals from the optocouplers.
        digitalWrite(opposite_sensing_pin, LOW);
        digitalWrite(sensing_pin, HIGH);
        delayMicroseconds(config::pop_up::SENSING_DELAY_US);

        const bool up_state = digitalRead(config::pins::UP_INPUT_PIN);
        const bool down_state = digitalRead(config::pins::DOWN_INPUT_PIN);

        if (up_state && !down_state)
        {
            ++up_votes;
        }
        else if (!up_state && down_state)
        {
            ++down_votes;
        }
        else
        {
            ++in_between_votes;
        }

        if ((i + 1u) < config::pop_up::SENSING_SAMPLE_COUNT &&
            config::pop_up::SENSING_SAMPLE_GAP_US > 0)
        {
            delayMicroseconds(config::pop_up::SENSING_SAMPLE_GAP_US);
        }
    }

    if (up_votes > down_votes && up_votes > in_between_votes)
    {
        return PopUpState::UP;
    }

    if (down_votes > up_votes && down_votes > in_between_votes)
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
void PopUp::_log_not_initialized_once() const
{
    if (init_warning_logged_)
    {
        return;
    }

    LOG("PopUp %s ignored call before begin().", name());
    init_warning_logged_ = true;
}

void PopUp::_start_pop_up()
{
    if (current_target == PopUpState::TIMEOUT)
    {
        LOG("Startup of %s prevented by pop-up being in timed-out state.", name());
        return;
    }
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
        const uint32_t move_duration_ms = static_cast<uint32_t>(millis() - movement_start_time);
        const char* reason = timed_out ? "TIMEOUT" : "normal";
        LOG("PopUp %s: Stopped motor. Reason=%s, Duration=%lu ms",
            name(),
            reason,
            static_cast<unsigned long>(move_duration_ms));
        statistics_manager.record_pop_up_cycle(pop_up_id, move_duration_ms);
    }
 
    if (timed_out)
    {
        LOG("PopUp %s: Starting to coast!", name());
        motor_controller->set_coast();
        report_pop_up_timeout(pop_up_id);
    }
    else
    { 
        LOG("PopUp %s: Starting to brake!", name());
        motor_controller->set_brake(true);
    }

    is_moving = false;
    winking = false;
    PopUpState new_target = (timed_out) ? PopUpState::TIMEOUT : PopUpState::IDLE;
    previous_target = current_target;
    current_target = new_target;
}

int PopUp::_get_sleepy_eye_move_time()
{
    constexpr uint8_t k_max_position = 6;
    float battery_voltage = read_battery_voltage();
    int expected_time_to_go_down_ms = timing_calibration.get_expected_down_time(battery_voltage);
    int k_min_move_time_ms = 150;
    int k_max_move_time_ms = expected_time_to_go_down_ms-150;
    int k_range_ms = k_max_move_time_ms - k_min_move_time_ms;

    sleepy_position_knob.update();

    uint8_t knob_position = sleepy_position_knob.get_position();
    if (knob_position > k_max_position)
    {
        knob_position = k_max_position;
    }

    LOG("Pop-up %s got expected time to go DOWN at %d ms from voltage %.1f", name(), expected_time_to_go_down_ms, battery_voltage);
    // Linear mapping: 0 -> 150 ms, 6 -> 550 ms
    return k_min_move_time_ms
         + (static_cast<int>(knob_position) * k_range_ms) / static_cast<int>(k_max_position);
}
