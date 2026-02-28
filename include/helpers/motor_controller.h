#pragma once
#include <Arduino.h>
#include "helpers/nmos_brake.h"
#include "esp_timer.h"
#include "freertos/portmacro.h"

class MotorController {
public:
  enum class mode : uint8_t {
    coast,   // PMOS off, NMOS off
    run,     // PMOS on,  NMOS off
    brake    // PMOS off, NMOS braking PWM
  };

  MotorController(int pmos_gate_pin,
                  bool pmos_active_low,
                  uint32_t deadtime_ms,
                  int nmos_pwm_pin,
                  int nmos_ledc_channel,
                  uint32_t nmos_pwm_freq_hz,
                  uint8_t nmos_pwm_resolution_bits,
                  float nmos_target_duty,
                  uint32_t nmos_ramp_time_us,
                  uint32_t nmos_step_period_us,
                  uint32_t nmos_hold_timpe_ms);

  bool begin();
  void end();

  mode get_mode() const;

  void set_coast();
  void set_run(bool enabled = true);
  void set_brake(bool enabled = true);

  void set_brake_target_duty(float duty);
  void set_brake_hold_time_ms(uint32_t hold_time_ms);

  bool is_braking() const;

  // Optional: true while waiting out deadtime before enabling run/brake
  bool is_in_deadtime() const;

private:
  int pmos_gate_pin_ = -1;
  bool pmos_active_low_ = true;
  uint32_t deadtime_ms_ = 1;

  NMOSBrake nmos_brake_;

  mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;

  mode mode_ = mode::coast;
  bool initialized_ = false;

  // Deadtime state
  esp_timer_handle_t deadtime_timer_ = nullptr;
  bool pending_ = false;
  mode pending_mode_ = mode::coast;

  void pmos_set_(bool on);
  void force_all_off_();

  void schedule_transition_locked_(mode target);   // target is run or brake
  void apply_pending_locked_();                    // called when deadtime expires

  // NMOS auto-stop hook -> set mode to coast
  static void nmos_auto_stop_thunk_(void* arg);
  void on_nmos_auto_stop_();

  // Deadtime timer hook
  static void deadtime_timer_thunk_(void* arg);
  void on_deadtime_timer_();
};
