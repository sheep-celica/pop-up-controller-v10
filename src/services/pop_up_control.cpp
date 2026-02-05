#include "services/pop_up_control.h"
#include "config.h"

// Main Pop-up classes
PopUp RH_POP_UP(config::pins::RH_MOTOR_PIN, config::pins::RH_SENSE_PIN);
PopUp LH_POP_UP(config::pins::LH_MOTOR_PIN, config::pins::LH_SENSE_PIN);


// Public functions
void update_pop_ups()
/*
Update the pop-ups if their current target is not IDLE
*/
{
    if (RH_POP_UP.get_target() != PopUpState::IDLE)
    {
        RH_POP_UP.update();
    }

    if (LH_POP_UP.get_target() != PopUpState::IDLE)
    {
        LH_POP_UP.update();
    }
}