#include "services/inputs/logic/toggle_button.h"
#include "services/inputs/logic/light_switch_up.h"
#include "services/inputs/logic/light_switch_hold.h"
#include "services/inputs/inputs_manager.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "config.h"


// ---------- Toggle Button (ESP32 GPIO) ----------
static InputPin toggle_button_pin {
    .backend = InputBackend::ESP32_GPIO,
    .esp32_pin = config::pins::TOGGLE_BUTTON_PIN
};

Input toggle_button(
    toggle_button_pin,
    true,    // active low
    30       // debounce ms
);

// ---------- Toggle Button Logic --------------
// Runs every loop AFTER all inputs have been updated by InputManager.
static void toggle_button_tick(uint32_t now_ms)
{
    if (toggle_button.released())
    {
        if (light_switch_up.is_high() || (light_switch_up.is_low() && light_switch_hold.is_low()))
        {
            // Wink
            LOG("Toggle button released - Winking");
            RH_POP_UP.wink_pop_up();
            LH_POP_UP.wink_pop_up();
        }
        else
        {
            // Toggle
            LOG("Toggle button released - Toggling");
            const PopUpState toggle_target_state =
            (RH_POP_UP.get_state() == PopUpState::DOWN && LH_POP_UP.get_state() == PopUpState::DOWN)
                ? PopUpState::UP
                : PopUpState::DOWN;
            safe_move_pop_up_to(&RH_POP_UP, toggle_target_state);
            safe_move_pop_up_to(&LH_POP_UP, toggle_target_state);
        }

    }
}

void toggle_button_register()
{
    // Register the input so it gets update() called exactly once per loop.
    inputs_manager.add(toggle_button);

    // Register this module's logic to run after updates.
    inputs_manager.add_task(toggle_button_tick);
}
