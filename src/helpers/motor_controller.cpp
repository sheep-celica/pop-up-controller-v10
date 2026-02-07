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
  portENTER_CRITICAL(&mux_);
  if (initialized_ && mode_ == mode::brake) {
    mode_ = mode::coast;  // bookkeeping only; outputs are already off by NMOSBrake
  }
  portEXIT_CRITICAL(&mux_);
}

void MotorController::deadtime_timer_thunk_(void* arg) {
  static_cast<MotorController*>(arg)->on_deadtime_timer_();
}

void MotorController::on_deadtime_timer_() {
  portENTER_CRITICAL(&mux_);
  if (!initialized_) {
    portEXIT_CRITICAL(&mux_);
    return;
  }
  apply_pending_locked_();
  portEXIT_CRITICAL(&mux_);
}

bool MotorController::begin() {
  if (pmos_gate_pin_ < 0) return false;

  // Avoid startup glitches: set OFF level before enabling output driver
  const uint8_t off_level = (false ^ pmos_active_low_) ? HIGH : LOW;
  digitalWrite(pmos_gate_pin_, off_level);
  pinMode(pmos_gate_pin_, OUTPUT);

  // Create deadtime timer
  {
    esp_timer_create_args_t args{};
    args.callback = &MotorController::deadtime_timer_thunk_;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "motor_deadtime";
    if (esp_timer_create(&args, &deadtime_timer_) != ESP_OK) return false;
  }

  // Start safe
  force_all_off_();

  if (!nmos_brake_.begin()) {
    force_all_off_();
    esp_timer_delete(deadtime_timer_);
    deadtime_timer_ = nullptr;
    return false;
  }

  // Auto-stop -> auto-coast bookkeeping
  nmos_brake_.set_on_auto_stop_callback(&MotorController::nmos_auto_stop_thunk_, this);

  portENTER_CRITICAL(&mux_);
  mode_ = mode::coast;
  pending_ = false;
  pending_mode_ = mode::coast;
  initialized_ = true;
  portEXIT_CRITICAL(&mux_);

  return true;
}

void MotorController::end() {
  portENTER_CRITICAL(&mux_);
  const bool was_init = initialized_;
  initialized_ = false;
  pending_ = false;
  portEXIT_CRITICAL(&mux_);
  if (!was_init) return;

  // Stop callbacks into us
  nmos_brake_.set_on_auto_stop_callback(nullptr, nullptr);

  if (deadtime_timer_) {
    (void)esp_timer_stop(deadtime_timer_);
  }

  force_all_off_();
  nmos_brake_.end();

  if (deadtime_timer_) {
    esp_timer_delete(deadtime_timer_);
    deadtime_timer_ = nullptr;
  }
}

MotorController::mode MotorController::get_mode() const {
  portENTER_CRITICAL(&mux_);
  const mode m = mode_;
  portEXIT_CRITICAL(&mux_);
  return m;
}

bool MotorController::is_in_deadtime() const {
  portENTER_CRITICAL(&mux_);
  const bool v = pending_;
  portEXIT_CRITICAL(&mux_);
  return v;
}

void MotorController::pmos_set_(bool on) {
  // Mutual exclusion: if PMOS is going on, NMOS must be off.
  if (on) {
    nmos_brake_.stop_braking(); // race-safe due to NMOSBrake locking
  }
  const uint8_t level = (on ^ pmos_active_low_) ? HIGH : LOW;
  digitalWrite(pmos_gate_pin_, level);
}

void MotorController::force_all_off_() {
  pmos_set_(false);
  nmos_brake_.stop_braking();
}

void MotorController::apply_pending_locked_() {
  if (!pending_) return;

  const mode target = pending_mode_;
  pending_ = false;
  pending_mode_ = mode::coast;

  // We are coming from a forced-all-off state, so this is safe to enable.
  if (target == mode::run) {
    pmos_set_(true);
    mode_ = mode::run;
  } else if (target == mode::brake) {
    // PMOS stays off
    nmos_brake_.start_braking();
    mode_ = mode::brake;
  } else {
    mode_ = mode::coast;
  }
}

void MotorController::schedule_transition_locked_(mode target) {
  // Cancel any prior pending transition
  if (deadtime_timer_) (void)esp_timer_stop(deadtime_timer_);

  // Immediately go to safe state
  force_all_off_();
  mode_ = mode::coast;

  pending_mode_ = target;
  pending_ = true;

  if (deadtime_ms_ == 0) {
    apply_pending_locked_();
    return;
  }

  if (deadtime_timer_) {
    (void)esp_timer_start_once(deadtime_timer_, (uint64_t)deadtime_ms_ * 1000ULL);
  }
}

void MotorController::set_coast() {
  portENTER_CRITICAL(&mux_);
  if (!initialized_) { portEXIT_CRITICAL(&mux_); return; }

  pending_ = false;
  pending_mode_ = mode::coast;
  if (deadtime_timer_) (void)esp_timer_stop(deadtime_timer_);

  force_all_off_();
  mode_ = mode::coast;
  portEXIT_CRITICAL(&mux_);
}

void MotorController::set_run(bool enabled) {
  portENTER_CRITICAL(&mux_);
  if (!initialized_) { portEXIT_CRITICAL(&mux_); return; }

  if (!enabled) {
    portEXIT_CRITICAL(&mux_);
    set_coast();
    return;
  }

  if (mode_ == mode::run && !pending_) {
    portEXIT_CRITICAL(&mux_);
    return;
  }

  schedule_transition_locked_(mode::run);
  portEXIT_CRITICAL(&mux_);
}

void MotorController::set_brake(bool enabled) {
  portENTER_CRITICAL(&mux_);
  if (!initialized_) { portEXIT_CRITICAL(&mux_); return; }

  if (!enabled) {
    portEXIT_CRITICAL(&mux_);
    set_coast();
    return;
  }

  if (mode_ == mode::brake && !pending_) {
    portEXIT_CRITICAL(&mux_);
    return;
  }

  schedule_transition_locked_(mode::brake);
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
