// helpers/nmos_brake.cpp
#include "helpers/nmos_brake.h"
#include <math.h>

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

void NMOSBrake::clamp_and_recalc_() {
  // Clamp duty
  if (target_duty_ < 0.0f) target_duty_ = 0.0f;
  if (target_duty_ > 1.0f) target_duty_ = 1.0f;

  // Ensure sane step period
  if (step_period_us_ == 0) step_period_us_ = 50;

  // Clamp resolution bits to avoid undefined shifts and invalid LEDC configs
  // (Arduino-ESP32 typically supports up to 15 bits depending on freq/timer)
  if (pwm_resolution_bits_ == 0) pwm_resolution_bits_ = 1;
  if (pwm_resolution_bits_ > 15) pwm_resolution_bits_ = 15;

  max_duty_ = (1u << pwm_resolution_bits_) - 1u;
  target_duty_counts_ = (uint32_t)lroundf(target_duty_ * (float)max_duty_);

  total_steps_ = ramp_time_us_ / step_period_us_;
  if (total_steps_ == 0) total_steps_ = 1;
}

bool NMOSBrake::begin() {
  if (pwm_pin_ < 0) return false;

  clamp_and_recalc_();

  // LEDC init (check for failure)
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

  initialized_ = true;
  return true;
}

void NMOSBrake::start_braking() {
  if (!initialized_) return;

  // Stop any prior activity first
  stop_timers_();

  // Allow callbacks to run
  abort_ = false;

  // Always restart from 0
  last_duty_ = 0;
  ledcWrite(ledc_channel_, 0);

  ramp_step_ = 0;
  ramping_ = true;
  braking_ = true;

  // Period is in microseconds
  esp_timer_start_periodic(ramp_timer_, step_period_us_);
}

void NMOSBrake::stop_braking() {
  if (!initialized_) return;

  // Tell callbacks to stop touching PWM ASAP
  abort_ = true;

  stop_timers_();

  ledcWrite(ledc_channel_, 0);
  last_duty_ = 0;

  braking_ = false;
  ramping_ = false;
}

void NMOSBrake::cancel_auto_stop() {
  if (!initialized_ || !hold_timer_) return;
  esp_timer_stop(hold_timer_);
}

void NMOSBrake::set_hold_time_ms(uint32_t hold_time_ms) {
  hold_time_ms_ = hold_time_ms;
}

void NMOSBrake::set_target_duty(float duty) {
  target_duty_ = duty;
  clamp_and_recalc_();
}

bool NMOSBrake::is_braking() const { return braking_; }
bool NMOSBrake::is_ramping() const { return ramping_; }

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

void NMOSBrake::stop_timers_() {
  if (ramp_timer_) esp_timer_stop(ramp_timer_);
  if (hold_timer_) esp_timer_stop(hold_timer_);
}

void NMOSBrake::on_ramp_timer_() {
  if (abort_) return;

  // Termination: once we reach total_steps_, set final duty and stop ramp timer.
  if (ramp_step_ >= total_steps_) {
    ledcWrite(ledc_channel_, target_duty_counts_);
    last_duty_ = target_duty_counts_;

    esp_timer_stop(ramp_timer_);
    ramping_ = false;

    if (hold_time_ms_ > 0 && hold_timer_) {
      esp_timer_start_once(hold_timer_, (uint64_t)hold_time_ms_ * 1000ULL);
    }
    return;
  }

  // Linear ramp 0 -> target over total_steps_
  uint32_t duty = (uint64_t)target_duty_counts_ * ramp_step_ / total_steps_;
  if (duty > target_duty_counts_) duty = target_duty_counts_;

  ledcWrite(ledc_channel_, duty);
  last_duty_ = duty;

  ramp_step_++;
}

void NMOSBrake::on_hold_timer_() {
  stop_braking();
}
