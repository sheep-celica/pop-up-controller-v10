#pragma once
#include <Arduino.h>
#include "helpers/nmos_brake.h"
#include "freertos/portmacro.h"

class MotorController {
public:
  enum class mode : uint8_t {
    coast,
    run,
    brake
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
                  uint32_t nmos_hold_time_ms);

  bool begin();
  void end();

  mode get_mode() const;

  void set_coast();
  void set_run(bool enabled = true);
  void set_brake(bool enabled = true);

  void set_brake_target_duty(float duty);
  void set_brake_hold_time_ms(uint32_t hold_time_ms);

  bool is_braking() const;

private:
  int pmos_gate_pin_ = -1;
  bool pmos_active_low_ = true;
  uint32_t deadtime_ms_ = 1;

  NMOSBrake nmos_brake_;

  // NEW: protects mode_ / initialized_ against timer-callback updates
  mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;

  mode mode_ = mode::coast;
  bool initialized_ = false;

  void apply_deadtime_();
  void pmos_set_(bool on);
  void force_all_off_();

  // NEW: NMOS auto-stop hook -> set mode to coast
  static void nmos_auto_stop_thunk_(void* arg);
  void on_nmos_auto_stop_();
};
