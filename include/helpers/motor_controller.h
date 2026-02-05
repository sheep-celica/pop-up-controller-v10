// motor_controller.h
#pragma once
#include <Arduino.h>
#include "helpers/nmos_brake.h"

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
                  uint32_t nmos_hold_time_ms);

  bool begin();
  void end();

  mode get_mode() const;

  void set_coast();
  void set_run(bool enabled = true);
  void set_brake(bool enabled = true);

  // Runtime tuning (optional)
  void set_brake_target_duty(float duty);
  void set_brake_hold_time_ms(uint32_t hold_time_ms);

  bool is_braking() const;

private:
  int pmos_gate_pin_ = -1;
  bool pmos_active_low_ = true;
  uint32_t deadtime_ms_ = 1;

  NMOSBrake nmos_brake_;

  mode mode_ = mode::coast;
  bool initialized_ = false;

  void apply_deadtime_();
  void pmos_set_(bool on);
  void force_all_off_();
};
