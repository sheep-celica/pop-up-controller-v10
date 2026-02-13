#include "helpers/nmos_brake.h"
#include <math.h>
#include "services/logging/logging.h"


NMOSBrake::NMOSBrake(int pwm_pin,
                     int ledc_channel,
                     uint32_t pwm_freq_hz,
                     uint8_t pwm_resolution_bits,
                     float target_duty,
                     uint32_t ramp_time_us,
                     uint32_t step_period_us,
                     uint32_t hold_time_ms)
: pwm_pin_(pwm_pin),
  ledc_channel_(ledc_channel),
  pwm_freq_hz_(pwm_freq_hz),
  pwm_resolution_bits_(pwm_resolution_bits),
  target_duty_(target_duty),
  ramp_time_us_(ramp_time_us),
  step_period_us_(step_period_us),
  hold_time_ms_(hold_time_ms) {}

void NMOSBrake::clamp_and_recalc_locked_() {
  // Clamp duty
  if (target_duty_ < 0.0f) target_duty_ = 0.0f;
  if (target_duty_ > 1.0f) target_duty_ = 1.0f;

  // Ensure sane step period
  if (step_period_us_ == 0) step_period_us_ = 50;

  // Clamp resolution bits (Arduino-ESP32 typically supports up to 15 bits depending on freq/timer)
  if (pwm_resolution_bits_ == 0) pwm_resolution_bits_ = 1;
  if (pwm_resolution_bits_ > 15) pwm_resolution_bits_ = 15;

  max_duty_ = (1u << pwm_resolution_bits_) - 1u;
  target_duty_counts_ = (uint32_t)lroundf(target_duty_ * (float)max_duty_);

  total_steps_ = ramp_time_us_ / step_period_us_;
  if (total_steps_ == 0) total_steps_ = 1;
}

void NMOSBrake::clamp_and_recalc_() {
  portENTER_CRITICAL(&mux_);
  clamp_and_recalc_locked_();
  portEXIT_CRITICAL(&mux_);
}

bool NMOSBrake::begin() {
  if (pwm_pin_ < 0) return false;

  // Precompute params
  clamp_and_recalc_();

  // LEDC init
  const double actual_freq = ledcSetup(ledc_channel_, pwm_freq_hz_, pwm_resolution_bits_);
  if (actual_freq == 0) return false;

  ledcAttachPin(pwm_pin_, ledc_channel_);
  ledcWrite(ledc_channel_, 0);

  // Ramp timer
  {
    esp_timer_create_args_t args{};
    args.callback = &NMOSBrake::ramp_timer_thunk;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "brake_ramp";
    if (esp_timer_create(&args, &ramp_timer_) != ESP_OK) return false;
  }

  // Hold timer
  {
    esp_timer_create_args_t args{};
    args.callback = &NMOSBrake::hold_timer_thunk;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "brake_hold";
    if (esp_timer_create(&args, &hold_timer_) != ESP_OK) return false;
  }

  portENTER_CRITICAL(&mux_);
  abort_ = true;
  braking_ = false;
  ramping_ = false;
  ramp_step_ = 0;
  last_duty_ = 0;
  portEXIT_CRITICAL(&mux_);

  initialized_ = true;
  return true;
}

void NMOSBrake::stop_timers_() {
  if (ramp_timer_) (void)esp_timer_stop(ramp_timer_);
  if (hold_timer_) (void)esp_timer_stop(hold_timer_);
}

void NMOSBrake::start_braking() {
  if (!initialized_) return;
  LOG("Starting to brake!");
  // Stop any prior activity first (OK if already stopped)
  stop_timers_();

  // Reset state + PWM under lock (prevents overlap with any in-flight callback)
  portENTER_CRITICAL(&mux_);
  abort_ = false;
  braking_ = true;
  ramping_ = true;

  ramp_step_ = 0;
  last_duty_ = 0;
  ledcWrite(ledc_channel_, 0);
  portEXIT_CRITICAL(&mux_);

  // Start periodic ramp timer
  const esp_err_t err = esp_timer_start_periodic(ramp_timer_, step_period_us_);
  if (err != ESP_OK) {
    // Fail safe: force off
    stop_braking();
  }
}

void NMOSBrake::stop_braking() {
  if (!initialized_) return;

  // 1) Prevent any future PWM writes from callbacks by setting abort_ under lock.
  //    If a callback is currently in its critical section, we wait here until it's done.
  portENTER_CRITICAL(&mux_);
  abort_ = true;

  // Force PWM off as the *last* LEDC write while holding the lock.
  ledcWrite(ledc_channel_, 0);
  last_duty_ = 0;
  ramp_step_ = 0;

  braking_ = false;
  ramping_ = false;
  portEXIT_CRITICAL(&mux_);

  // 2) Stop timers (callbacks may still fire, but they will not write PWM because abort_ is true)
  stop_timers_();
}

void NMOSBrake::cancel_auto_stop() {
  if (!initialized_ || !hold_timer_) return;
  (void)esp_timer_stop(hold_timer_);
}

void NMOSBrake::set_hold_time_ms(uint32_t hold_time_ms) {
  portENTER_CRITICAL(&mux_);
  hold_time_ms_ = hold_time_ms;
  portEXIT_CRITICAL(&mux_);
}

void NMOSBrake::set_target_duty(float duty) {
  portENTER_CRITICAL(&mux_);
  target_duty_ = duty;
  clamp_and_recalc_locked_();
  portEXIT_CRITICAL(&mux_);
}

bool NMOSBrake::is_braking() const {
  portENTER_CRITICAL(&mux_);
  const bool v = braking_;
  portEXIT_CRITICAL(&mux_);
  return v;
}

bool NMOSBrake::is_ramping() const {
  portENTER_CRITICAL(&mux_);
  const bool v = ramping_;
  portEXIT_CRITICAL(&mux_);
  return v;
}

void NMOSBrake::end() {
  if (!initialized_) return;

  stop_braking();

  if (ramp_timer_) { esp_timer_delete(ramp_timer_); ramp_timer_ = nullptr; }
  if (hold_timer_) { esp_timer_delete(hold_timer_); hold_timer_ = nullptr; }

  initialized_ = false;
}

void NMOSBrake::ramp_timer_thunk(void* arg) {
  static_cast<NMOSBrake*>(arg)->on_ramp_timer_();
}

void NMOSBrake::hold_timer_thunk(void* arg) {
  static_cast<NMOSBrake*>(arg)->on_hold_timer_();
}

void NMOSBrake::on_ramp_timer_() {
  bool stop_ramp = false;
  bool start_hold = false;
  uint64_t hold_us = 0;

  portENTER_CRITICAL(&mux_);

  if (abort_) {
    portEXIT_CRITICAL(&mux_);
    return;
  }

  const uint32_t local_total_steps = total_steps_;
  const uint32_t local_target = target_duty_counts_;
  const uint32_t local_hold_ms = hold_time_ms_;

  if (ramp_step_ >= local_total_steps) {
    // Final duty
    ledcWrite(ledc_channel_, local_target);
    last_duty_ = local_target;

    ramping_ = false;
    stop_ramp = true;

    if (local_hold_ms > 0 && hold_timer_) {
      start_hold = true;
      hold_us = (uint64_t)local_hold_ms * 1000ULL;
    }

    portEXIT_CRITICAL(&mux_);

    // Stop ramp timer outside lock
    if (stop_ramp && ramp_timer_) (void)esp_timer_stop(ramp_timer_);

    // Start hold timer outside lock
    if (start_hold && hold_timer_) (void)esp_timer_start_once(hold_timer_, hold_us);

    return;
  }

  // Linear ramp 0 -> target over total_steps_
  uint32_t duty = (uint64_t)local_target * ramp_step_ / local_total_steps;
  if (duty > local_target) duty = local_target;

  ledcWrite(ledc_channel_, duty);
  last_duty_ = duty;
  ramp_step_++;

  portEXIT_CRITICAL(&mux_);
}

void NMOSBrake::set_on_auto_stop_callback(auto_stop_cb_t cb, void* arg) {
  portENTER_CRITICAL(&mux_);
  auto_stop_cb_ = cb;
  auto_stop_arg_ = arg;
  portEXIT_CRITICAL(&mux_);
}

void NMOSBrake::on_hold_timer_() {
  // Auto stop after hold
  stop_braking();

  // Fire callback (if any) - this runs in ESP timer task context.
  auto_stop_cb_t cb = nullptr;
  void* arg = nullptr;

  portENTER_CRITICAL(&mux_);
  cb = auto_stop_cb_;
  arg = auto_stop_arg_;
  portEXIT_CRITICAL(&mux_);

  if (cb) cb(arg);
}