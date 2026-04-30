#include "verification/positioning_test.h"

#include <Arduino.h>

#include "config.h"
#include "services/logging/logging.h"
#include "verification/io_expander_test.h"

namespace {
    enum class PositionState : uint8_t {
        Unknown,
        Down,
        Up,
        InBetween,
    };

    struct PositionSample {
        PositionState state;
        bool up_input;
        bool down_input;
    };

    PositionState s_last_rh_state = PositionState::Unknown;
    PositionState s_last_lh_state = PositionState::Unknown;
    constexpr uint32_t kPositionChangeBlinkMs = 75;

    const char* position_state_name(PositionState state)
    {
        switch (state) {
            case PositionState::Down: return "DOWN";
            case PositionState::Up: return "UP";
            case PositionState::InBetween: return "INBETWEEN";
            case PositionState::Unknown: return "UNKNOWN";
        }

        return "UNKNOWN";
    }

    PositionState decode_position(bool up_input, bool down_input)
    {
        if (up_input && !down_input) {
            return PositionState::Down;
        }

        if (!up_input && down_input) {
            return PositionState::Up;
        }

        return PositionState::InBetween;
    }

    PositionSample read_selected_position(bool rh_selected)
    {
        digitalWrite(config::pins::RH_SENSE_PIN, rh_selected ? HIGH : LOW);
        digitalWrite(config::pins::LH_SENSE_PIN, rh_selected ? LOW : HIGH);

        delayMicroseconds(200);

        const bool up_input = digitalRead(config::pins::UP_INPUT_PIN) == HIGH;
        const bool down_input = digitalRead(config::pins::DOWN_INPUT_PIN) == HIGH;

        return {
            decode_position(up_input, down_input),
            up_input,
            down_input,
        };
    }

    void print_position_change(
        const char* name,
        PositionSample sample,
        PositionState& last_state,
        IoExpanderPin indicator_led)
    {
        if (sample.state == last_state) {
            return;
        }

        last_state = sample.state;
        LOG(
            "%s position changed: %s (UP=%u DOWN=%u).",
            name,
            position_state_name(sample.state),
            sample.up_input ? 1u : 0u,
            sample.down_input ? 1u : 0u);

        blink_internal_expander_led(indicator_led, kPositionChangeBlinkMs);
    }
}

void setup_positioning_test()
{
    digitalWrite(config::pins::RH_SENSE_PIN, LOW);
    digitalWrite(config::pins::LH_SENSE_PIN, LOW);
    pinMode(config::pins::RH_SENSE_PIN, OUTPUT);
    pinMode(config::pins::LH_SENSE_PIN, OUTPUT);

    pinMode(config::pins::UP_INPUT_PIN, INPUT);
    pinMode(config::pins::DOWN_INPUT_PIN, INPUT);

    LOG("Positioning verification started. Turn motors by hand and watch RH/LH position changes.");
}

void update_positioning_test()
{
    print_position_change(
        "RH",
        read_selected_position(true),
        s_last_rh_state,
        config::pins::internal_expander::ERROR_LED_PIN);
    print_position_change(
        "LH",
        read_selected_position(false),
        s_last_lh_state,
        config::pins::internal_expander::STATUS_LED_PIN);
}
