// helpers/nmos_brake.h
#pragma once
#include <Arduino.h>
#include "esp_timer.h"

class NMOSBrake {
public:
  NMOSBrake(int pwm_pin,
            int ledc_channel,
            uint32_t pwm_freq_hz,
            uint8_t pwm_resolution_bits,
            float target_duty,
            uint32_t ramp_time_us,
            uint32_t step_period_us,
            uint32_t hold_time_ms);

  bool begin();

  void start_braking();      // always from 0
  void stop_braking();
  void cancel_auto_stop();

  void set_hold_time_ms(uint32_t hold_time_ms);
  void set_target_duty(float duty);

  bool is_braking() const;
  bool is_ramping() const;

  void end();

private:
  int pwm_pin_ = -1;
  int ledc_channel_ = 0;
  uint32_t pwm_freq_hz_ = 20000;
  uint8_t pwm_resolution_bits_ = 10;

  float target_duty_ = 0.6f;
  uint32_t ramp_time_us_ = 5000;
  uint32_t step_period_us_ = 50;
  uint32_t hold_time_ms_ = 0;

  bool initialized_ = false;
  volatile bool abort_ = false;
  volatile bool braking_ = false;
  volatile bool ramping_ = false;

  esp_timer_handle_t ramp_timer_ = nullptr;
  esp_timer_handle_t hold_timer_ = nullptr;

  uint32_t max_duty_ = 0;
  uint32_t target_duty_counts_ = 0;
  uint32_t total_steps_ = 1;

  volatile uint32_t ramp_step_ = 0;
  volatile uint32_t last_duty_ = 0;

  static void ramp_timer_thunk(void* arg);
  static void hold_timer_thunk(void* arg);

  void stop_timers_();
  void on_ramp_timer_();
  void on_hold_timer_();

  void clamp_and_recalc_();
};
