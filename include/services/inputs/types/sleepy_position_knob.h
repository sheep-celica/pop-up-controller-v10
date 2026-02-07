#pragma once

#include <Arduino.h>

class SleepyPositionKnob
{
public:
    explicit SleepyPositionKnob(gpio_num_t analog_pin);

    void update();
    uint8_t get_position() const;

private:
    gpio_num_t analog_pin;
    int raw_value;
    uint8_t position;
    uint8_t last_position;

    static constexpr uint8_t sample_count = 8;

    uint8_t classify_position(int adc_value) const;
};


extern SleepyPositionKnob sleepy_position_knob;