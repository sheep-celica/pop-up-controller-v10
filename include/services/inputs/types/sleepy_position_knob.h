#pragma once

#include <Arduino.h>

class SleepyPositionKnob
{
public:
    explicit SleepyPositionKnob(gpio_num_t analog_pin);

    void update();
    uint8_t get_position() const;
    void set_position_override(bool enabled, uint8_t position);
    bool is_position_override_enabled() const;
    uint8_t get_position_override() const;

private:
    gpio_num_t analog_pin;
    int raw_value;
    uint8_t position;
    uint8_t last_position;
    bool position_override_enabled;
    uint8_t position_override;

    static constexpr uint8_t sample_count = 8;

    uint8_t classify_position(int adc_value) const;
};


extern SleepyPositionKnob sleepy_position_knob;
