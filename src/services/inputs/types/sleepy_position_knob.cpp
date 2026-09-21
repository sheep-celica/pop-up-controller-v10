#include "services/inputs/types/sleepy_position_knob.h"
#include "services/logging/logging.h"
#include "config.h"

// Global instance 
SleepyPositionKnob sleepy_position_knob(config::pins::SLEEPY_EYE_KNOB_PIN);

SleepyPositionKnob::SleepyPositionKnob(gpio_num_t analog_pin)
    : analog_pin(analog_pin),
      raw_value(0),
      position(0),
      last_position(0),
      position_override_enabled(false),
      position_override(config::pop_up::DEFAULT_SLEEPY_EYE_POSITION_OVERRIDE)
{
}

void SleepyPositionKnob::update()
{
    if (position_override_enabled)
    {
        last_position = position;
        position = position_override;
        return;
    }

    uint32_t sum = 0;

    for (uint8_t i = 0; i < sample_count; ++i)
    {
        sum += analogRead(analog_pin);
    }

    raw_value = static_cast<int>(sum / sample_count);

    last_position = position;
    position = classify_position(raw_value);
}

void SleepyPositionKnob::set_position_override(bool enabled, uint8_t override_position)
{
    position_override_enabled = enabled;
    position_override = override_position;
}

bool SleepyPositionKnob::is_position_override_enabled() const
{
    return position_override_enabled;
}

uint8_t SleepyPositionKnob::get_position_override() const
{
    return position_override;
}

uint8_t SleepyPositionKnob::get_position() const
{
    return position;
}

uint8_t SleepyPositionKnob::classify_position(int adc_value) const
{
    if (adc_value > 2989) return 0;
    if (adc_value > 1543) return 1;
    if (adc_value > 1028) return 2;
    if (adc_value > 755)  return 3;
    if (adc_value > 590)  return 4;
    if (adc_value > 473)  return 5;
    return 6;
}
