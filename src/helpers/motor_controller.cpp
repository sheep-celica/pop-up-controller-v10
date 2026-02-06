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

void MotorController::nmos_auto_stop_thunk_(void* arg) {
  static_cast<MotorController*>(arg)->on_nmos_auto_stop_();
}

void MotorController::on_nmos_auto_stop_() {
  // NMOS already stopped itself. We only want mode bookkeeping to reflect "coast".
  portENTER_CRITICAL(&mux_);
  if (initialized_ && mode_ == mode::brake) {
    mode_ = mode::coast;
  }
  portEXIT_CRITICAL(&mux_);
}

bool MotorController::begin() {
  if (pmos_gate_pin_ < 0) return false;

  const uint8_t off_level = (false ^ pmos_active_low_) ? HIGH : LOW;
  digitalWrite(pmos_gate_pin_, off_level);
  pinMode(pmos_gate_pin_, OUTPUT);

  force_all_off_();

  if (!nmos_brake_.begin()) {
    force_all_off_();
    return false;
  }

  // Register auto-stop -> auto-coast bookkeeping
  nmos_brake_.set_on_auto_stop_callback(&MotorController::nmos_auto_stop_thunk_, this);

  portENTER_CRITICAL(&mux_);
  mode_ = mode::coast;
  initialized_ = true;
  portEXIT_CRITICAL(&mux_);

  return true;
}

void MotorController::end() {
  portENTER_CRITICAL(&mux_);
  const bool was_init = initialized_;
  initialized_ = false;
  portEXIT_CRITICAL(&mux_);

  if (!was_init) return;

  // Prevent any late callback from touching us (belt-and-suspenders)
  nmos_brake_.set_on_auto_stop_callback(nullptr, nullptr);

  force_all_off_();
  nmos_brake_.end();
}

MotorController::mode MotorController::get_mode() const {
  portENTER_CRITICAL(&mux_);
  const mode m = mode_;
  portEXIT_CRITICAL(&mux_);
  return m;
}

void MotorController::apply_deadtime_() {
  if (deadtime_ms_ > 0) delay(deadtime_ms_);
}

void MotorController::pmos_set_(bool on) {
  if (on) {
    nmos_brake_.stop_braking(); // now race-safe
  }
  const uint8_t level = (on ^ pmos_active_low_) ? HIGH : LOW;
  digitalWrite(pmos_gate_pin_, level);
}

void MotorController::force_all_off_() {
  pmos_set_(false);
  nmos_brake_.stop_braking();
}

void MotorController::set_coast() {
  portENTER_CRITICAL(&mux_);
  const bool ok = initialized_;
  portEXIT_CRITICAL(&mux_);
  if (!ok) return;

  force_all_off_();

  portENTER_CRITICAL(&mux_);
  mode_ = mode::coast;
  portEXIT_CRITICAL(&mux_);
}

void MotorController::set_run(bool enabled) {
  portENTER_CRITICAL(&mux_);
  const bool ok = initialized_;
  const mode cur = mode_;
  portEXIT_CRITICAL(&mux_);
  if (!ok) return;

  if (!enabled) { set_coast(); return; }
  if (cur == mode::run) return;

  force_all_off_();
  apply_deadtime_();

  pmos_set_(true);

  portENTER_CRITICAL(&mux_);
  mode_ = mode::run;
  portEXIT_CRITICAL(&mux_);
}

void MotorController::set_brake(bool enabled) {
  portENTER_CRITICAL(&mux_);
  const bool ok = initialized_;
  const mode cur = mode_;
  portEXIT_CRITICAL(&mux_);
  if (!ok) return;

  if (!enabled) { set_coast(); return; }
  if (cur == mode::brake) return;

  force_all_off_();
  apply_deadtime_();

  nmos_brake_.start_braking();

  portENTER_CRITICAL(&mux_);
  mode_ = mode::brake;
  portEXIT_CRITICAL(&mux_);
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
