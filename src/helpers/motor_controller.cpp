// motor_controller.cpp
#include "helpers/motor_controller.h"


MotorController::MotorController(int pmos_gate_pin,
                                 bool pmos_active_low,
                                 uint32_t deadtime_ms,
                                 int nmos_pwm_pin,
                                 int nmos_ledc_channel,
                                 uint32_t nmos_pwm_freq_hz,
                                 uint8_t nmos_pwm_resolution_bits,
                                 float nmos_target_duty,
                                 uint32_t nmos_ramp_time_us,
                                 uint32_t nmos_step_period_us,
                                 uint32_t nmos_hold_time_ms)
: pmos_gate_pin_(pmos_gate_pin),
  pmos_active_low_(pmos_active_low),
  deadtime_ms_(deadtime_ms),
  nmos_brake_(nmos_pwm_pin,
              nmos_ledc_channel,
              nmos_pwm_freq_hz,
              nmos_pwm_resolution_bits,
              nmos_target_duty,
              nmos_ramp_time_us,
              nmos_step_period_us,
              nmos_hold_time_ms) {}

bool MotorController::begin() {
  if (pmos_gate_pin_ < 0) return false;

  pinMode(pmos_gate_pin_, OUTPUT);

  // Start from a known safe state
  force_all_off_();

  if (!nmos_brake_.begin()) {
    force_all_off_();
    return false;
  }

  mode_ = mode::coast;
  initialized_ = true;
  return true;
}

void MotorController::end() {
  if (!initialized_) return;
  force_all_off_();
  nmos_brake_.end();
  initialized_ = false;
}

MotorController::mode MotorController::get_mode() const {
  return mode_;
}

void MotorController::apply_deadtime_() {
  if (deadtime_ms_ > 0) delay(deadtime_ms_);
}

void MotorController::pmos_set_(bool on) {
  // Enforce mutual exclusion: if PMOS is going on, NMOS must be off.
  if (on) {
    nmos_brake_.stop_braking();
  }

  // Active-low handling: if active-low, ON => LOW; else ON => HIGH
  const uint8_t level = (on ^ pmos_active_low_) ? HIGH : LOW;
  digitalWrite(pmos_gate_pin_, level);
}

void MotorController::force_all_off_() {
  // Turn off PMOS immediately
  pmos_set_(false);

  // Stop NMOS PWM immediately
  nmos_brake_.stop_braking();
}

void MotorController::set_coast() {
  if (!initialized_) return;
  force_all_off_();
  mode_ = mode::coast;
}

void MotorController::set_run(bool enabled) {
  if (!initialized_) return;

  if (!enabled) {
    set_coast();
    return;
  }

  if (mode_ == mode::run) return;

  // Break-before-make: ensure both are off, wait deadtime, then enable PMOS only.
  force_all_off_();
  apply_deadtime_();

  pmos_set_(true);
  mode_ = mode::run;
}

void MotorController::set_brake(bool enabled) {
  if (!initialized_) return;

  if (!enabled) {
    set_coast();
    return;
  }

  if (mode_ == mode::brake) return;

  // Break-before-make: ensure both are off, wait deadtime, then enable NMOS braking only.
  force_all_off_();
  apply_deadtime_();

  // PMOS remains off
  nmos_brake_.start_braking();
  mode_ = mode::brake;
}

void MotorController::set_brake_target_duty(float duty) {
  nmos_brake_.set_target_duty(duty);
}

void MotorController::set_brake_hold_time_ms(uint32_t hold_time_ms) {
  nmos_brake_.set_hold_time_ms(hold_time_ms);
}

bool MotorController::is_braking() const {
  return nmos_brake_.is_braking();
}
