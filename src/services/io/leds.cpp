#include "services/io/leds.h"
#include "services/io/io_expanders.h"
#include <Arduino.h>
#include <math.h>
#include "services/logging/logging.h"

// Choose a free LEDC channel for the illumination PWM
static const int LEDC_CHANNEL_ILLUM = 2;

void setup_leds()
{
    // Ensure digital outputs are configured for the three internal expander LEDs
    internal_ads.setDigitalOutput(static_cast<uint8_t>(config::pins::internal_expander::STATUS_LED_PIN), true);
    internal_ads.setDigitalOutput(static_cast<uint8_t>(config::pins::internal_expander::INPUT_LED_PIN), true);
    internal_ads.setDigitalOutput(static_cast<uint8_t>(config::pins::internal_expander::ERROR_LED_PIN), true);

    // Apply the cached pin config to the ADS7138 so the pin modes take effect
    (void)internal_ads.applyPinConfig();

    // Default all three to off
    internal_ads.digitalWrite(static_cast<uint8_t>(config::pins::internal_expander::STATUS_LED_PIN), false);
    internal_ads.digitalWrite(static_cast<uint8_t>(config::pins::internal_expander::INPUT_LED_PIN), false);
    internal_ads.digitalWrite(static_cast<uint8_t>(config::pins::internal_expander::ERROR_LED_PIN), false);

    // Setup LEDC for illumination PWM using illumination-specific params
    const double actual_freq = ledcSetup(LEDC_CHANNEL_ILLUM,
                                        config::pins::illumination::FREQUENCY_HZ,
                                        config::pins::illumination::PWM_RESOLUTION_BITS);
    (void)actual_freq; // keep compiler quiet if unused
    ledcAttachPin(config::pins::ILLUMINATION_ON_PIN, LEDC_CHANNEL_ILLUM);
    ledcWrite(LEDC_CHANNEL_ILLUM, 0);
}

static uint8_t ledIdToPin(LedId led)
{
    switch (led) {
        case LedId::STATUS_LED: return static_cast<uint8_t>(config::pins::internal_expander::STATUS_LED_PIN);
        case LedId::INPUT_LED:  return static_cast<uint8_t>(config::pins::internal_expander::INPUT_LED_PIN);
        case LedId::ERROR_LED:  return static_cast<uint8_t>(config::pins::internal_expander::ERROR_LED_PIN);
    }
    return static_cast<uint8_t>(config::pins::internal_expander::STATUS_LED_PIN);
}

void set_led_state(LedId led, bool on)
{
    const uint8_t ch = ledIdToPin(led);
    internal_ads.digitalWrite(ch, on);
}

void turn_on_illumination()
{
    // Read potentiometer that adjusts LED brightness
    const float volts = internal_ads.readAnalogVolts(static_cast<uint8_t>(config::pins::internal_expander::LED_ADJUST_POT_PIN));
    // ADS default AVDD is 3.3V; clamp and compute ratio
    const float avdd = 3.3f;
    float ratio = 0.0f;
    if (volts > 0.0f) ratio = volts / avdd;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    const uint32_t max_duty = (1u << config::pop_up::braking::PWM_RESOLUTION_BITS) - 1u;
    const uint32_t duty = (uint32_t)lroundf(ratio * (float)max_duty);
    LOG("illum pot volts=%.3f, ratio=%.3f, duty=%u", volts, ratio, duty);
    ledcWrite(LEDC_CHANNEL_ILLUM, duty);
}

void turn_off_illumination()
{
    ledcWrite(LEDC_CHANNEL_ILLUM, 0);
}
