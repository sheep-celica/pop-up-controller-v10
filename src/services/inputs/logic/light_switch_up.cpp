#include "services/inputs/logic/light_switch_up.h"
#include "services/inputs/inputs_manager.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "config.h"


// ---------- Light-switch UP (ESP32 GPIO) ----------
static InputPin light_switch_up_pin {
    .backend = InputBackend::ESP32_GPIO,
    .esp32_pin = config::pins::LIGHT_SWITCH_UP_PIN
};

Input light_switch_up(
    light_switch_up_pin,
    true,    // active low
    30       // debounce ms
);

// ---------- Light-switch UP Logic --------------
// Runs every loop AFTER all inputs have been updated by InputManager.
static void light_switch_up_tick(uint32_t now_ms)
{   
    if (light_switch_up.pressed())
    {
        LOG("Light switch UP pressed");
    }
    if (light_switch_up.released())
    {
        LOG("Light switch UP released");
    }
    if (light_switch_up.is_high() && light_switch_up.get_stable_state_time() > config::pop_up::DELAY_TO_GO_UP_MS)
    {
        // Move Pop-ups UP if Light-switch has been in the HEAD position or flashed for at least config::pop_up::DELAY_TO_GO_UP_MS ms
        // The delay is to ensure no weird noise causes the Pop-ups to suddenly react without user input
        safe_move_pop_up_to(&RH_POP_UP, PopUpState::UP);
        safe_move_pop_up_to(&LH_POP_UP, PopUpState::UP);
    }
}

void light_switch_up_register()
{
    // Register the input so it gets update() called exactly once per loop.
    inputs_manager.add(light_switch_up);

    // Register this module's logic to run after updates.
    inputs_manager.add_task(light_switch_up_tick);
}