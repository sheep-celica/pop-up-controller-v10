#include "services/io/leds.h"
#include "services/io/io_expanders.h"
#include <Arduino.h>
#include <math.h>
#include "services/logging/logging.h"

// Illumination LEDC channel is configured in `config.h` under pins::illumination::LEDC_CHANNEL_ILLUM

struct DigitalLedBlinkState {
    bool steady_on = false;          // requested steady state when not blinking
    bool actual_on = false;          // last state written to hardware
    bool blinking = false;
    uint32_t remaining_toggles = 0;  // one blink = two toggles
    uint32_t half_period_ms = 0;
    unsigned long last_toggle_ms = 0;
};

static constexpr uint8_t kDigitalLedCount = 4;
static DigitalLedBlinkState s_digital_leds[kDigitalLedCount];
static const LedId kDigitalLedIds[kDigitalLedCount] = {
    LedId::STATUS_LED,
    LedId::INPUT_LED,
    LedId::ERROR_LED,
    LedId::SLEEPY_EYE_STATUS
};

static bool s_startup_lamp_test_active = false;
static unsigned long s_startup_lamp_test_end_ms = 0;

// Ramping state for illumination PWM
static uint32_t s_current_duty = 0; // current duty written to LEDC
static uint32_t s_target_duty = 0;  // desired duty we are ramping to
static unsigned long s_last_update_ms = 0;
static bool s_ramping = false;
static bool s_requested_on = false; // logical ON/OFF target requested by API (default OFF)

static uint8_t ledIdToPin(LedId led);

static uint8_t ledIndex(LedId led)
{
    switch (led) {
        case LedId::STATUS_LED:        return 0;
        case LedId::INPUT_LED:         return 1;
        case LedId::ERROR_LED:         return 2;
        case LedId::SLEEPY_EYE_STATUS: return 3;
    }
    return 0;
}

static void write_digital_led(LedId led, bool on)
{
    const uint8_t idx = ledIndex(led);
    s_digital_leds[idx].actual_on = on;
    internal_ads.digitalWrite(ledIdToPin(led), on);
}

static void end_startup_lamp_test(unsigned long now)
{
    s_startup_lamp_test_active = false;

    for (uint8_t i = 0; i < kDigitalLedCount; ++i) {
        DigitalLedBlinkState& st = s_digital_leds[i];
        const LedId led = kDigitalLedIds[i];

        if (st.blinking && st.half_period_ms > 0) {
            // Make queued blink requests start immediately after the lamp test ends.
            st.last_toggle_ms = now - st.half_period_ms;
            continue;
        }

        write_digital_led(led, st.steady_on);
    }
}

static void stop_blink_and_restore(LedId led)
{
    const uint8_t idx = ledIndex(led);
    DigitalLedBlinkState& st = s_digital_leds[idx];
    st.blinking = false;
    st.remaining_toggles = 0;
    st.half_period_ms = 0;
    write_digital_led(led, st.steady_on);
}

static void update_blink_state(LedId led, unsigned long now)
{
    DigitalLedBlinkState& st = s_digital_leds[ledIndex(led)];
    if (!st.blinking || st.half_period_ms == 0) {
        return;
    }

    while (st.blinking && (now - st.last_toggle_ms) >= st.half_period_ms) {
        st.last_toggle_ms += st.half_period_ms;

        if (st.remaining_toggles == 0) {
            stop_blink_and_restore(led);
            return;
        }

        write_digital_led(led, !st.actual_on);
        st.remaining_toggles--;

        if (st.remaining_toggles == 0) {
            stop_blink_and_restore(led);
            return;
        }
    }
}

void setup_leds()
{
    // Ensure digital outputs are configured for the internal expander LEDs
    internal_ads.setDigitalOutput(static_cast<uint8_t>(config::pins::internal_expander::STATUS_LED_PIN), true);
    internal_ads.setDigitalOutput(static_cast<uint8_t>(config::pins::internal_expander::INPUT_LED_PIN), true);
    internal_ads.setDigitalOutput(static_cast<uint8_t>(config::pins::internal_expander::ERROR_LED_PIN), true);
    internal_ads.setDigitalOutput(static_cast<uint8_t>(config::pins::internal_expander::SLEEPY_EYE_LED_PIN), true);

    // Apply the cached pin config to the ADS7138 so the pin modes take effect
    (void)internal_ads.applyPinConfig();

    for (uint8_t i = 0; i < kDigitalLedCount; ++i) {
        s_digital_leds[i] = {};
    }

    // Default steady-state behavior:
    // - Status LED stays ON when idle
    // - Other LEDs stay OFF when idle
    s_digital_leds[ledIndex(LedId::STATUS_LED)].steady_on = true;

    // Startup lamp test: force all digital LEDs on for 2 seconds (non-blocking)
    for (uint8_t i = 0; i < kDigitalLedCount; ++i) {
        write_digital_led(kDigitalLedIds[i], true);
    }
    s_startup_lamp_test_active = true;
    s_startup_lamp_test_end_ms = millis() + 2000UL;

    // Setup LEDC for illumination PWM using illumination-specific params
    const double actual_freq = ledcSetup(config::pins::illumination::LEDC_CHANNEL_ILLUM,
                                         config::pins::illumination::FREQUENCY_HZ,
                                         config::pins::illumination::PWM_RESOLUTION_BITS);
    (void)actual_freq; // keep compiler quiet if unused
    ledcAttachPin(config::pins::ILLUMINATION_ON_PIN, config::pins::illumination::LEDC_CHANNEL_ILLUM);
    ledcWrite(config::pins::illumination::LEDC_CHANNEL_ILLUM, 0);

    // initialize ramping state
    s_current_duty = 0;
    s_target_duty = 0;
    s_last_update_ms = millis();
    s_ramping = false;
    s_requested_on = false;
}

static uint8_t ledIdToPin(LedId led)
{
    switch (led) {
        case LedId::STATUS_LED:        return static_cast<uint8_t>(config::pins::internal_expander::STATUS_LED_PIN);
        case LedId::INPUT_LED:         return static_cast<uint8_t>(config::pins::internal_expander::INPUT_LED_PIN);
        case LedId::ERROR_LED:         return static_cast<uint8_t>(config::pins::internal_expander::ERROR_LED_PIN);
        case LedId::SLEEPY_EYE_STATUS: return static_cast<uint8_t>(config::pins::internal_expander::SLEEPY_EYE_LED_PIN);
    }
    return static_cast<uint8_t>(config::pins::internal_expander::STATUS_LED_PIN);
}

void set_led_state(LedId led, bool on)
{
    DigitalLedBlinkState& st = s_digital_leds[ledIndex(led)];
    st.steady_on = on;

    // Explicit state writes override any queued blinking.
    st.blinking = false;
    st.remaining_toggles = 0;
    st.half_period_ms = 0;

    if (s_startup_lamp_test_active) {
        return;
    }

    write_digital_led(led, on);
}

void blink_led(LedId led, uint32_t times, float frequency_hz)
{
    if (times == 0 || frequency_hz <= 0.0f) {
        return;
    }

    uint32_t half_period_ms = (uint32_t)lroundf(500.0f / frequency_hz);
    if (half_period_ms == 0) {
        half_period_ms = 1;
    }

    DigitalLedBlinkState& st = s_digital_leds[ledIndex(led)];
    const uint32_t add_toggles = times * 2u;

    if (st.blinking) {
        // Accumulate more blinks and apply the latest frequency to upcoming toggles.
        if (UINT32_MAX - st.remaining_toggles < add_toggles) {
            st.remaining_toggles = UINT32_MAX;
        } else {
            st.remaining_toggles += add_toggles;
        }
        st.half_period_ms = half_period_ms;
        return;
    }

    st.blinking = true;
    st.half_period_ms = half_period_ms;
    st.last_toggle_ms = millis();
    st.remaining_toggles = add_toggles;

    if (s_startup_lamp_test_active) {
        return;
    }

    // Start immediately by toggling once, then continue in update_leds().
    write_digital_led(led, !st.actual_on);
    if (st.remaining_toggles > 0) {
        st.remaining_toggles--;
    }

    if (st.remaining_toggles == 0) {
        stop_blink_and_restore(led);
    }
}

void turn_on_illumination()
{
    // If already requested ON, do nothing
    if (s_requested_on) return;

    s_requested_on = true;
    // Read potentiometer that adjusts LED brightness
    const float volts = internal_ads.readAnalogVolts(static_cast<uint8_t>(config::pins::internal_expander::LED_ADJUST_POT_PIN));
    // ADS default AVDD is 3.3V; clamp and compute ratio
    const float avdd = 3.3f;
    float ratio = 0.0f;
    if (volts > 0.0f) ratio = volts / avdd;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    // Apply perceptual gamma correction so the pot feels roughly linear to the human eye
    float adjusted = 0.0f;
    if (ratio > 0.0f) {
        const float inv_gamma = 1.0f / config::pins::illumination::GAMMA;
        adjusted = powf(ratio, inv_gamma);
        if (adjusted < 0.0f) adjusted = 0.0f;
        if (adjusted > 1.0f) adjusted = 1.0f;
    }

    const uint32_t max_duty = (1u << config::pins::illumination::PWM_RESOLUTION_BITS) - 1u;
    const uint32_t duty = (uint32_t)lroundf(adjusted * (float)max_duty);
    LOG("Turning ON illumination. pot volts=%.3f, ratio=%.3f, adjusted=%.3f, target duty=%u", volts, ratio, adjusted, duty);

    // Start ramping to the new target duty
    s_target_duty = duty;
    s_ramping = (s_current_duty != s_target_duty);
    s_last_update_ms = millis();
}

void turn_off_illumination()
{
    // If already requested OFF, do nothing
    if (!s_requested_on) return;
    LOG("Turning OFF illumination.");
    s_requested_on = false;

    // Start ramping down to zero from current duty
    s_target_duty = 0;
    s_ramping = (s_current_duty != 0);
    s_last_update_ms = millis();
}

void update_leds()
{
    const unsigned long now = millis();

    if (s_startup_lamp_test_active && (int32_t)(now - s_startup_lamp_test_end_ms) >= 0) {
        end_startup_lamp_test(now);
    }

    if (!s_startup_lamp_test_active) {
        update_blink_state(LedId::STATUS_LED, now);
        update_blink_state(LedId::INPUT_LED, now);
        update_blink_state(LedId::ERROR_LED, now);
        update_blink_state(LedId::SLEEPY_EYE_STATUS, now);
    }

    unsigned long elapsed = now - s_last_update_ms;
    if (elapsed == 0) return;
    s_last_update_ms = now;

    if (!s_ramping) return;

    const int32_t diff = (int32_t)s_target_duty - (int32_t)s_current_duty;
    if (diff == 0) { s_ramping = false; return; }

    const uint32_t ramp_ms = config::pins::illumination::RAMP_TIME_MS;
    if (ramp_ms == 0) {
        s_current_duty = s_target_duty;
        ledcWrite(config::pins::illumination::LEDC_CHANNEL_ILLUM, s_current_duty);
        s_ramping = false;
        return;
    }

    float step_fraction = (float)elapsed / (float)ramp_ms;
    if (step_fraction < 0.0f) step_fraction = 0.0f;
    if (step_fraction > 1.0f) step_fraction = 1.0f;

    float new_duty_f = (float)s_current_duty + step_fraction * (float)diff;
    if (diff > 0 && new_duty_f > (float)s_target_duty) new_duty_f = (float)s_target_duty;
    if (diff < 0 && new_duty_f < (float)s_target_duty) new_duty_f = (float)s_target_duty;

    uint32_t new_duty;
    // For downramps (diff < 0), use floor() to ensure duty always decreases monotonically
    // and doesn't get stuck due to rounding up small fractional decreases
    if (diff < 0) {
        new_duty = (uint32_t)floorf(new_duty_f);
    } else {
        new_duty = (uint32_t)lroundf(new_duty_f);
    }

    if (new_duty != s_current_duty) {
        s_current_duty = new_duty;
        ledcWrite(config::pins::illumination::LEDC_CHANNEL_ILLUM, s_current_duty);
    }

    if (s_current_duty == s_target_duty) s_ramping = false;
}
