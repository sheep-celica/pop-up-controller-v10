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

