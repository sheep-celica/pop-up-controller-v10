#include "services/inputs/logic/light_switch_hold.h"
#include "services/inputs/logic/light_switch_up.h"
#include "services/inputs/inputs_manager.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "config.h"


// ---------- Light-switch HOLD (ESP32 GPIO) ----------
static InputPin light_switch_hold_pin {
    .backend = InputBackend::ESP32_GPIO,
    .esp32_pin = config::pins::LIGHT_SWITCH_HOLD_PIN
};

Input light_switch_hold(
    light_switch_hold_pin,
    true,    // active low
    30       // debounce ms
);

// ---------- Light-switch HOLD Logic --------------
// Runs every loop AFTER all inputs have been updated by InputManager.
static void light_switch_hold_tick(uint32_t now_ms)
{
    if (light_switch_hold.pressed())
    {
        LOG("Light switch HOLD pressed");
    }
    if (light_switch_hold.released())
    {
        LOG("Light switch HOLD released");
    }
    if (light_switch_up.is_low() && light_switch_hold.is_low() && light_switch_hold.get_stable_state_time() > config::pop_up::DELAY_TO_GO_DOWN_MS && light_switch_up.get_stable_state_time() > config::pop_up::DELAY_TO_GO_DOWN_MS)
    {
        // Move Pop-ups DOWN if Light-switch is in the OFF position for over config::pop_up::DELAY_TO_GO_DOWN_MS
        // The delay is to ensure no weird noise causes the Pop-ups to suddenly react without user input
        safe_move_pop_up_to(&RH_POP_UP, PopUpState::DOWN);
        safe_move_pop_up_to(&LH_POP_UP, PopUpState::DOWN);
    }
}

void light_switch_hold_register()
{
    // Register the input so it gets update() called exactly once per loop.
    inputs_manager.add(light_switch_hold);

    // Register this module's logic to run after updates.
    inputs_manager.add_task(light_switch_hold_tick);
}
