#pragma once

#include <Arduino.h>


/**
 * @brief Represents the state of the PopUp mechanism
 */
enum class PopUpState : uint8_t
{
    DOWN = 0,
    UP = 1,
    IN_BETWEEN = 2,
    IDLE = 3,
    TIMEOUT = 4
};

inline const char* pop_up_state_name(PopUpState state)
{
    switch (state)
    {
        case PopUpState::UP:            return "UP";
        case PopUpState::DOWN:          return "DOWN";
        case PopUpState::IN_BETWEEN:    return "IN_BETWEEN";
        case PopUpState::IDLE:          return "IDLE";
        case PopUpState::TIMEOUT:       return "TIMEOUT";
        default:                        return "Unknown";
    }
}