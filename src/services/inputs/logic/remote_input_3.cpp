#include "services/inputs/logic/remote_input_3.h"
#include "services/inputs/logic/light_switch_up.h"
#include "services/inputs/inputs_manager.h"
#include "services/inputs/remote_input_pins.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "config.h"


// ---------- Remote Input 3 (External Exapnder GPIO) ----------
static InputPin remote_input_3_pin {
    .backend = InputBackend::EXTERNAL_EXPANDER,
    .expander_pin = config::pins::external_expander::REMOTE_INPUT_2
};

Input remote_input_3(
    remote_input_3_pin,
    true,    // active low
    50       // debounce ms
);

// ---------- Remote Input 3 Logic --------------
// Runs every loop AFTER all inputs have been updated by InputManager.
static void remote_input_3_button_tick(uint32_t now_ms)
{
    (void)now_ms;

    if (
        remote_input_3.released() &&
        (is_light_switch_safely_off() || are_remote_inputs_with_headlights_allowed()))
    {
        statistics_manager.record_remote_input_press(3);
        LOG("Winking Both Pop-ups - Remote");
        RH_POP_UP.wink_pop_up();
        LH_POP_UP.wink_pop_up();
    }
}

void remote_input_3_button_register()
{
    // Register the input so it gets update() called exactly once per loop.
    inputs_manager.add(remote_input_3);

    // Register this module's logic to run after updates.
    inputs_manager.add_task(remote_input_3_button_tick);
}
