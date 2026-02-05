#include "helpers/nmos_brake.h"
#include <math.h>

NMOSBrake::NMOSBrake(const Config& cfg)
: cfg_(cfg) {}

bool NMOSBrake::begin() {
  if (cfg_.pwmPin < 0) return false;

  // Clamp target duty
  if (cfg_.targetDuty < 0.0f) cfg_.targetDuty = 0.0f;
  if (cfg_.targetDuty > 1.0f) cfg_.targetDuty = 1.0f;

  // LEDC init
  ledcSetup(cfg_.ledcChannel, cfg_.pwmFreqHz, cfg_.pwmResolutionBits);
  ledcAttachPin(cfg_.pwmPin, cfg_.ledcChannel);
  ledcWrite(cfg_.ledcChannel, 0);

  maxDuty_ = (1u << cfg_.pwmResolutionBits) - 1u;
  targetDutyCounts_ = (uint32_t)lroundf(cfg_.targetDuty * (float)maxDuty_);

  if (cfg_.stepPeriodUs == 0) cfg_.stepPeriodUs = 50;

  totalSteps_ = cfg_.rampTimeUs / cfg_.stepPeriodUs;
  if (totalSteps_ == 0) totalSteps_ = 1;

  // Ramp timer
  {
    esp_timer_create_args_t args{};
    args.callback = &NMOSBrake::rampTimerThunk;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "brake_ramp";
    if (esp_timer_create(&args, &rampTimer_) != ESP_OK) return false;
  }

  // Hold timer
  {
    esp_timer_create_args_t args{};
    args.callback = &NMOSBrake::holdTimerThunk;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "brake_hold";
    if (esp_timer_create(&args, &holdTimer_) != ESP_OK) return false;
  }

  initialized_ = true;
  return true;
}

void NMOSBrake::start_braking(bool restartFromZero) {
  if (!initialized_) return;

  stopTimers_();
  abort_ = false;

  if (restartFromZero) {
    lastDuty_ = 0;
    ledcWrite(cfg_.ledcChannel, 0);
  }

  rampStep_ = 0;
  ramping_ = true;
  braking_ = true;

  esp_timer_start_periodic(rampTimer_, cfg_.stepPeriodUs);
}

void NMOSBrake::stop_braking() {
  if (!initialized_) return;

  abort_ = true;
  stopTimers_();

  ledcWrite(cfg_.ledcChannel, 0);
  lastDuty_ = 0;

  braking_ = false;
  ramping_ = false;
}

void NMOSBrake::cancel_auto_stop() {
  if (!initialized_ || !holdTimer_) return;
  esp_timer_stop(holdTimer_);
}

void NMOSBrake::set_hold_time_ms(uint32_t holdTimeMs) {
  cfg_.holdTimeMs = holdTimeMs;
}

void NMOSBrake::set_target_duty(float duty) {
  if (duty < 0.0f) duty = 0.0f;
  if (duty > 1.0f) duty = 1.0f;
  cfg_.targetDuty = duty;
  targetDutyCounts_ = (uint32_t)lroundf(cfg_.targetDuty * (float)maxDuty_);
}

bool NMOSBrake::is_braking() const { return braking_; }
bool NMOSBrake::is_ramping() const { return ramping_; }

void NMOSBrake::end() {
  if (!initialized_) return;

  stop_braking();

  if (rampTimer_) { esp_timer_delete(rampTimer_); rampTimer_ = nullptr; }
  if (holdTimer_) { esp_timer_delete(holdTimer_); holdTimer_ = nullptr; }

  initialized_ = false;
}

void NMOSBrake::rampTimerThunk(void* arg) {
  static_cast<NMOSBrake*>(arg)->onRampTimer_();
}

void NMOSBrake::holdTimerThunk(void* arg) {
  static_cast<NMOSBrake*>(arg)->onHoldTimer_();
}

void NMOSBrake::stopTimers_() {
  if (rampTimer_) esp_timer_stop(rampTimer_);
  if (holdTimer_) esp_timer_stop(holdTimer_);
}

void NMOSBrake::onRampTimer_() {
  if (abort_) return;

  // Linear ramp 0 -> target over totalSteps_
  uint32_t duty = (uint64_t)targetDutyCounts_ * rampStep_ / totalSteps_;
  if (duty > targetDutyCounts_) duty = targetDutyCounts_;

  ledcWrite(cfg_.ledcChannel, duty);
  lastDuty_ = duty;

  rampStep_++;

  if (rampStep_ > totalSteps_) {
    ledcWrite(cfg_.ledcChannel, targetDutyCounts_);
    lastDuty_ = targetDutyCounts_;

    esp_timer_stop(rampTimer_);
    ramping_ = false;

    if (cfg_.holdTimeMs > 0 && holdTimer_) {
      esp_timer_start_once(holdTimer_, (uint64_t)cfg_.holdTimeMs * 1000ULL);
    }
  }
}

void NMOSBrake::onHoldTimer_() {
  stop_braking();
}
