#include "helpers/DRV8243.h"

namespace {
    uint8_t gpio_to_pin(gpio_num_t pin)
    {
        return static_cast<uint8_t>(pin);
    }
}

DRV8243::DRV8243(const Config& config)
    : config_(config)
{
}

bool DRV8243::begin()
{
    set_outputs_safe_();
    pinMode(config_.control_pin, OUTPUT);
    pinMode(config_.coast_disable_pin, OUTPUT);
    pinMode(config_.current_pin, INPUT);

    ledcSetup(config_.ledc_channel, config_.pwm_frequency_hz, config_.pwm_resolution_bits);
    ledcAttachPin(gpio_to_pin(config_.control_pin), config_.ledc_channel);
    ledcWrite(config_.ledc_channel, 0);

    initialized_ = true;
    mode_ = Mode::Coast;
    enabled_ = false;
    reset_stall_tracking_();
    return true;
}

void DRV8243::update(uint32_t now_ms)
{
    if (!initialized_) {
        return;
    }

    if (mode_ == Mode::Run && safe_start_until_ms_ != 0 && static_cast<int32_t>(now_ms - safe_start_until_ms_) >= 0) {
        safe_start_until_ms_ = 0;
        write_duty_percent_(requested_duty_percent_);
    }

    // TODO: Revision D stall detection belongs here once PopUp integration defines
    // how stalls should be reported, latched, and mapped to timeout/error behavior.
    (void)check_for_stall(now_ms);
}

void DRV8243::wake_up_impulse()
{
    digitalWrite(config_.sleep_pin, LOW);
    pinMode(config_.sleep_pin, OUTPUT);
    delay(1);

    digitalWrite(config_.sleep_pin, HIGH);
    delay(config_.driver_ready_delay_ms);

    noInterrupts();
    digitalWrite(config_.sleep_pin, LOW);
    delayMicroseconds(config_.nsleep_reset_pulse_us);
    digitalWrite(config_.sleep_pin, HIGH);
    interrupts();

    delay(config_.driver_ready_delay_ms);
}

void DRV8243::enable()
{
    if (!initialized_) {
        return;
    }

    wake_up_impulse();
    enabled_ = true;
    if (mode_ == Mode::Disabled) {
        coast();
    }
}

void DRV8243::disable()
{
    if (!initialized_) {
        return;
    }

    set_outputs_safe_();
    requested_duty_percent_ = 0.0f;
    applied_duty_percent_ = 0.0f;
    safe_start_until_ms_ = 0;
    enabled_ = false;
    mode_ = Mode::Disabled;
    reset_stall_tracking_();
}

void DRV8243::run(float duty_percent)
{
    if (!initialized_) {
        return;
    }

    if (!enabled_) {
        enable();
    }

    requested_duty_percent_ = clamp_duty_percent_(duty_percent);
    run_started_ms_ = millis();
    reset_stall_tracking_();

    digitalWrite(config_.coast_disable_pin, LOW);
    delay(1);

    const bool should_safe_start =
        config_.safe_start_enabled &&
        requested_duty_percent_ > 0.0f &&
        requested_duty_percent_ < config_.safe_start_duty_percent &&
        config_.safe_start_duration_ms > 0;

    if (should_safe_start) {
        safe_start_until_ms_ = run_started_ms_ + config_.safe_start_duration_ms;
        write_duty_percent_(config_.safe_start_duty_percent);
    } else {
        safe_start_until_ms_ = 0;
        write_duty_percent_(requested_duty_percent_);
    }

    mode_ = Mode::Run;
}

void DRV8243::coast()
{
    if (!initialized_) {
        return;
    }

    ledcWrite(config_.ledc_channel, 0);
    digitalWrite(config_.coast_disable_pin, HIGH);
    applied_duty_percent_ = 0.0f;
    safe_start_until_ms_ = 0;
    mode_ = enabled_ ? Mode::Coast : Mode::Disabled;
    reset_stall_tracking_();
}

void DRV8243::brake()
{
    if (!initialized_) {
        return;
    }

    ledcWrite(config_.ledc_channel, 0);
    digitalWrite(config_.coast_disable_pin, LOW);
    applied_duty_percent_ = 0.0f;
    safe_start_until_ms_ = 0;
    mode_ = enabled_ ? Mode::Brake : Mode::Disabled;
    reset_stall_tracking_();
}

bool DRV8243::check_for_stall(uint32_t now_ms)
{
    (void)now_ms;

    // TODO: Revision D stall detection will live here. The intended shape is:
    // ignore the configured startup blanking window, require current above the
    // configured threshold for the configured duration, then report the latched
    // stall to the future PopUp/motor interface integration.
    return false;
}

float DRV8243::read_current_a() const
{
    const float raw = static_cast<float>(read_current_raw());
    const float adc_voltage = (raw * config_.adc_reference_v) / config_.adc_max_raw;
    const float ipropi_voltage = adc_voltage / adc_divider_ratio_();
    const float ipropi_current_a = ipropi_voltage / effective_ipropi_ohms_();
    return ipropi_current_a * config_.ipropi_scaling_a_per_a;
}

uint16_t DRV8243::read_current_raw() const
{
    return static_cast<uint16_t>(analogRead(config_.current_pin));
}

DRV8243::Mode DRV8243::mode() const
{
    return mode_;
}

bool DRV8243::enabled() const
{
    return enabled_;
}

float DRV8243::requested_duty_percent() const
{
    return requested_duty_percent_;
}

float DRV8243::applied_duty_percent() const
{
    return applied_duty_percent_;
}

float DRV8243::adc_divider_ratio_() const
{
    return config_.adc_to_gnd_ohms / (config_.ipropi_to_adc_ohms + config_.adc_to_gnd_ohms);
}

float DRV8243::effective_ipropi_ohms_() const
{
    return 1.0f / ((1.0f / config_.ipropi_to_gnd_ohms) +
                   (1.0f / (config_.ipropi_to_adc_ohms + config_.adc_to_gnd_ohms)));
}

float DRV8243::clamp_duty_percent_(float duty_percent) const
{
    if (duty_percent < 0.0f) {
        return 0.0f;
    }

    if (duty_percent > 100.0f) {
        return 100.0f;
    }

    return duty_percent;
}

uint32_t DRV8243::duty_percent_to_raw_(float duty_percent) const
{
    const uint32_t max_duty = (1UL << config_.pwm_resolution_bits) - 1UL;
    return static_cast<uint32_t>((clamp_duty_percent_(duty_percent) * static_cast<float>(max_duty)) / 100.0f);
}

void DRV8243::write_duty_percent_(float duty_percent)
{
    applied_duty_percent_ = clamp_duty_percent_(duty_percent);
    ledcWrite(config_.ledc_channel, duty_percent_to_raw_(applied_duty_percent_));
}

void DRV8243::set_outputs_safe_()
{
    digitalWrite(config_.control_pin, LOW);
    digitalWrite(config_.coast_disable_pin, HIGH);
}

void DRV8243::reset_stall_tracking_()
{
    stall_over_threshold_since_ms_ = 0;
}
