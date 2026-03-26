#include "services/pop_up_control/pop_up_control.h"
#include "services/logging/logging.h"
#include "services/inputs/logic/light_switch_up.h"
#include "services/io/leds.h"
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
  config::pop_up::braking::TARGET_DUTY_CYCLE_RATIO,         // nmos_target_duty (0..1)
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
  config::pop_up::braking::TARGET_DUTY_CYCLE_RATIO,
  config::pop_up::braking::BRAKING_TIME_US,
  config::pop_up::braking::STEP_PERIOD_US,
  config::pop_up::braking::HOLD_TIME_MS
);

PopUp RH_POP_UP(&RH_MOTOR, config::pins::RH_SENSE_PIN, PopUpId::RH);
PopUp LH_POP_UP(&LH_MOTOR, config::pins::LH_SENSE_PIN, PopUpId::LH);

Preferences RH_PREFS;
Preferences LH_PREFS;

namespace {
  constexpr const char* kSleepyEyeConfigNamespace = "sleepy_cfg";
  constexpr const char* kAllowSleepyEyeWithHeadlightsKey = "allow_hd";
  constexpr const char* kPopUpRuntimeConfigNamespace = "popup_cfg";
  constexpr const char* kMinStatePersistMsKey = "min_p_ms";
  constexpr const char* kSensingDelayUsKey = "sns_d_us";

  Preferences s_sleepy_eye_config_preferences;
  bool s_sleepy_eye_config_initialized = false;
  bool s_allow_sleepy_eye_with_headlights_loaded = false;
  bool s_allow_sleepy_eye_with_headlights = false;

  Preferences s_pop_up_runtime_config_preferences;
  bool s_pop_up_runtime_config_initialized = false;
  bool s_pop_up_runtime_config_loaded = false;
  uint32_t s_pop_up_min_state_persist_ms = config::pop_up::MIN_STATE_PERSIST_MS;
  uint32_t s_pop_up_sensing_delay_us = config::pop_up::SENSING_DELAY_US;

  void apply_pop_up_runtime_config()
  {
    RH_POP_UP.set_min_state_persist_ms(s_pop_up_min_state_persist_ms);
    LH_POP_UP.set_min_state_persist_ms(s_pop_up_min_state_persist_ms);
    RH_POP_UP.set_sensing_delay_us(s_pop_up_sensing_delay_us);
    LH_POP_UP.set_sensing_delay_us(s_pop_up_sensing_delay_us);
  }

  void ensure_sleepy_eye_config_loaded()
  {
    if (!s_sleepy_eye_config_initialized)
    {
      s_sleepy_eye_config_preferences.begin(kSleepyEyeConfigNamespace, false);
      s_sleepy_eye_config_initialized = true;
    }

    if (!s_allow_sleepy_eye_with_headlights_loaded)
    {
      if (s_sleepy_eye_config_preferences.isKey(kAllowSleepyEyeWithHeadlightsKey))
      {
        s_allow_sleepy_eye_with_headlights = s_sleepy_eye_config_preferences.getBool(
          kAllowSleepyEyeWithHeadlightsKey,
          false);
      }
      else
      {
        s_allow_sleepy_eye_with_headlights = false;
      }

      s_allow_sleepy_eye_with_headlights_loaded = true;
    }
  }

  void ensure_pop_up_runtime_config_loaded()
  {
    if (!s_pop_up_runtime_config_initialized)
    {
      s_pop_up_runtime_config_preferences.begin(kPopUpRuntimeConfigNamespace, false);
      s_pop_up_runtime_config_initialized = true;
    }

    if (!s_pop_up_runtime_config_loaded)
    {
      if (s_pop_up_runtime_config_preferences.isKey(kMinStatePersistMsKey))
      {
        s_pop_up_min_state_persist_ms = s_pop_up_runtime_config_preferences.getUInt(
          kMinStatePersistMsKey,
          config::pop_up::MIN_STATE_PERSIST_MS);
      }
      else
      {
        s_pop_up_min_state_persist_ms = config::pop_up::MIN_STATE_PERSIST_MS;
      }

      if (s_pop_up_runtime_config_preferences.isKey(kSensingDelayUsKey))
      {
        s_pop_up_sensing_delay_us = s_pop_up_runtime_config_preferences.getUInt(
          kSensingDelayUsKey,
          config::pop_up::SENSING_DELAY_US);
      }
      else
      {
        s_pop_up_sensing_delay_us = config::pop_up::SENSING_DELAY_US;
      }

      apply_pop_up_runtime_config();
      s_pop_up_runtime_config_loaded = true;
    }
  }
}


// Public functions
void setup_pop_ups()
/*
To be run once in the beginning to perform initial configuration of pop-ups.
*/
{
  RH_MOTOR.begin();
  LH_MOTOR.begin();
  RH_POP_UP.begin();
  LH_POP_UP.begin();
  ensure_pop_up_runtime_config_loaded();

  // Load calibrations
  RH_PREFS.begin(timing_calibration_namespace(PopUpId::RH), false);
  LH_PREFS.begin(timing_calibration_namespace(PopUpId::LH), false);
  RH_POP_UP.timing_calibration.load_from_preferences(RH_PREFS);
  LH_POP_UP.timing_calibration.load_from_preferences(LH_PREFS);

  ensure_sleepy_eye_config_loaded();
  LOG(
    "ALLOW_SLEEPY_EYE_MODE_WITH_HEADLIGHTS=%s",
    s_allow_sleepy_eye_with_headlights ? "TRUE" : "FALSE");
  LOG(
    "MIN_STATE_PERSIST_MS=%lu",
    static_cast<unsigned long>(s_pop_up_min_state_persist_ms));
  LOG(
    "SENSING_DELAY_US=%lu",
    static_cast<unsigned long>(s_pop_up_sensing_delay_us));
}

void update_pop_ups()
/*
Update the pop-ups while active, plus a short startup force-poll window.
*/
{
    const bool force_polling_active = millis() < config::pop_up::FORCE_POLL_PERIOD_MS;

    if (RH_POP_UP.get_target() != PopUpState::IDLE || force_polling_active)
    {
        RH_POP_UP.update();
    }

    if (LH_POP_UP.get_target() != PopUpState::IDLE || force_polling_active)
    {
        LH_POP_UP.update();
    }
}

bool are_pop_ups_idle_or_timed_out()
{
    const PopUpState rh_target = RH_POP_UP.get_target();
    const PopUpState lh_target = LH_POP_UP.get_target();

    const bool rh_idle_or_timed_out =
        rh_target == PopUpState::IDLE || rh_target == PopUpState::TIMEOUT;
    const bool lh_idle_or_timed_out =
        lh_target == PopUpState::IDLE || lh_target == PopUpState::TIMEOUT;

    return rh_idle_or_timed_out && lh_idle_or_timed_out;
}

void safe_move_pop_up_to(PopUp *pop_up, PopUpState target)
{
  if (pop_up->get_target() == PopUpState::TIMEOUT || pop_up->is_winking() || pop_up->get_sleepy_eye_mode())
  {
    // Do not set target if pop-up is timed out, winking or in sleepy eye mode.
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

bool toggle_sleepy_eye_mode()
{
  ensure_sleepy_eye_config_loaded();
  const bool sleepy_eye_mode_on = !RH_POP_UP.get_sleepy_eye_mode();

  if (sleepy_eye_mode_on && !s_allow_sleepy_eye_with_headlights && !is_light_switch_safely_off())
  {
    LOG("Sleepy eye mode toggle blocked: headlights are active.");
    LOG("Set ALLOW_SLEEPY_EYE_MODE_WITH_HEADLIGHTS to TRUE to override.");
    return false;
  }

  LOG("Toggling sleepy eye mode to %d", sleepy_eye_mode_on ? 1 : 0);
  RH_POP_UP.set_sleepy_eye_mode(sleepy_eye_mode_on);
  LH_POP_UP.set_sleepy_eye_mode(sleepy_eye_mode_on);
  set_led_state(LedId::SLEEPY_EYE_STATUS, sleepy_eye_mode_on);
  return true;
}

bool is_sleepy_eye_mode_with_headlights_allowed()
{
  ensure_sleepy_eye_config_loaded();
  return s_allow_sleepy_eye_with_headlights;
}

bool set_sleepy_eye_mode_with_headlights_allowed(bool allowed)
{
  ensure_sleepy_eye_config_loaded();

  const size_t bytes_written = s_sleepy_eye_config_preferences.putBool(
    kAllowSleepyEyeWithHeadlightsKey,
    allowed);
  if (bytes_written != sizeof(uint8_t))
  {
    return false;
  }

  s_allow_sleepy_eye_with_headlights = allowed;
  s_allow_sleepy_eye_with_headlights_loaded = true;
  return true;
}

uint32_t get_pop_up_min_state_persist_ms()
{
  ensure_pop_up_runtime_config_loaded();
  return s_pop_up_min_state_persist_ms;
}

bool set_pop_up_min_state_persist_ms(uint32_t min_state_persist_ms)
{
  ensure_pop_up_runtime_config_loaded();

  const size_t bytes_written = s_pop_up_runtime_config_preferences.putUInt(
    kMinStatePersistMsKey,
    min_state_persist_ms);
  if (bytes_written != sizeof(uint32_t))
  {
    return false;
  }

  s_pop_up_min_state_persist_ms = min_state_persist_ms;
  s_pop_up_runtime_config_loaded = true;
  apply_pop_up_runtime_config();
  return true;
}

uint32_t get_pop_up_sensing_delay_us()
{
  ensure_pop_up_runtime_config_loaded();
  return s_pop_up_sensing_delay_us;
}

bool set_pop_up_sensing_delay_us(uint32_t sensing_delay_us)
{
  ensure_pop_up_runtime_config_loaded();

  const size_t bytes_written = s_pop_up_runtime_config_preferences.putUInt(
    kSensingDelayUsKey,
    sensing_delay_us);
  if (bytes_written != sizeof(uint32_t))
  {
    return false;
  }

  s_pop_up_sensing_delay_us = sensing_delay_us;
  s_pop_up_runtime_config_loaded = true;
  apply_pop_up_runtime_config();
  return true;
}
