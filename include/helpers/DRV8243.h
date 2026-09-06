#pragma once

#include <Arduino.h>
#include <cstdint>

class DRV8243 {
public:
    enum class Mode : uint8_t {
        Disabled,
        Coast,
        Run,
        Brake,
    };

    struct Config {
        gpio_num_t control_pin;
        gpio_num_t coast_disable_pin;
        gpio_num_t current_pin;
        gpio_num_t sleep_pin;
        uint8_t ledc_channel;
        uint32_t pwm_frequency_hz;
        uint8_t pwm_resolution_bits;
        uint32_t nsleep_reset_pulse_us;
        uint32_t driver_ready_delay_ms;
        float adc_reference_v;
        float adc_max_raw;
        float ipropi_scaling_a_per_a;
        float ipropi_to_gnd_ohms;
        float ipropi_to_adc_ohms;
        float adc_to_gnd_ohms;
        bool safe_start_enabled;
        float safe_start_duty_percent;
        uint32_t safe_start_duration_ms;
        float stall_current_a;
        uint32_t stall_duration_ms;
        uint32_t stall_startup_blanking_ms;
    };

    explicit DRV8243(const Config& config);

    bool begin();
    void update(uint32_t now_ms);

    void wake_up_impulse();
    void enable();
    void disable();
    void run(float duty_percent);
    void coast();
    void brake();

    bool check_for_stall(uint32_t now_ms);
    float read_current_a() const;
    float read_current_a_uncalibrated() const;
    float current_a_from_raw(uint16_t raw) const;
    float uncalibrated_current_a_from_raw(uint16_t raw) const;
    uint16_t read_current_raw() const;

    void set_current_calibration(float scale, float offset_a);
    float current_calibration_scale() const;
    float current_calibration_offset_a() const;

    void set_stall_protection_enabled(bool enabled);
    bool stall_protection_enabled() const;
    void set_stall_config(
        float current_a,
        uint32_t duration_ms,
        uint32_t startup_blanking_ms);
    float stall_current_a() const;
    uint32_t stall_duration_ms() const;
    uint32_t stall_startup_blanking_ms() const;
    bool consume_stall_fault();
    void clear_stall_fault();

    Mode mode() const;
    bool enabled() const;
    float requested_duty_percent() const;
    float applied_duty_percent() const;

private:
    Config config_;
    Mode mode_ = Mode::Disabled;
    bool initialized_ = false;
    bool enabled_ = false;
    float requested_duty_percent_ = 0.0f;
    float applied_duty_percent_ = 0.0f;
    uint32_t run_started_ms_ = 0;
    uint32_t safe_start_until_ms_ = 0;
    uint32_t stall_over_threshold_since_ms_ = 0;
    float current_calibration_scale_ = 1.0f;
    float current_calibration_offset_a_ = 0.0f;
    bool stall_protection_enabled_ = true;
    bool stall_fault_ = false;

    float adc_divider_ratio_() const;
    float effective_ipropi_ohms_() const;
    float clamp_duty_percent_(float duty_percent) const;
    uint32_t duty_percent_to_raw_(float duty_percent) const;
    void write_duty_percent_(float duty_percent);
    void set_outputs_safe_();
    void reset_stall_tracking_();
};
