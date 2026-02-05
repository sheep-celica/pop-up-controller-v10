#pragma once
#include <Arduino.h>
#include "esp_timer.h"

class NMOSBrake {
public:
  struct Config {
    int pwmPin = -1;
    int ledcChannel = 0;
    uint32_t pwmFreqHz = 20000;
    uint8_t pwmResolutionBits = 10;

    float targetDuty = 0.60f;        // 0..1
    uint32_t rampTimeUs = 5000;      // total ramp time
    uint32_t stepPeriodUs = 50;      // update period

    uint32_t holdTimeMs = 0;         // 0 disables auto-stop
  };

  explicit NMOSBrake(const Config& cfg);

  // Call once from setup()
  bool begin();

  // Start ramping braking. restartFromZero=true sets duty to 0 before ramp
  void start_braking(bool restartFromZero = true);

  // Immediate stop (duty=0), abort ramp/hold
  void stop_braking();

  // Cancel auto-stop timer if running
  void cancel_auto_stop();

  // Runtime changes
  void set_hold_time_ms(uint32_t holdTimeMs);
  void set_target_duty(float duty);

  bool is_braking() const;
  bool is_ramping() const;

  void end();

private:
  Config cfg_;

  bool initialized_ = false;
  volatile bool abort_ = false;
  volatile bool braking_ = false;
  volatile bool ramping_ = false;

  esp_timer_handle_t rampTimer_ = nullptr;
  esp_timer_handle_t holdTimer_ = nullptr;

  uint32_t maxDuty_ = 0;
  uint32_t targetDutyCounts_ = 0;
  uint32_t totalSteps_ = 1;

  volatile uint32_t rampStep_ = 0;
  volatile uint32_t lastDuty_ = 0;

  static void rampTimerThunk(void* arg);
  static void holdTimerThunk(void* arg);

  void stopTimers_();
  void onRampTimer_();
  void onHoldTimer_();
};
