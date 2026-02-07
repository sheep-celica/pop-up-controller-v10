#include "services/inputs/logic/remote_input_2.h"
#include "services/inputs/inputs_manager.h"
#include "services/logging.h"
#include "services/pop_up_control.h"
#include "config.h"


// ---------- Remote Input 2 (External Exapnder GPIO) ----------
static InputPin remote_input_2_pin {
    .backend = InputBackend::EXTERNAL_EXPANDER,
    .expander_pin = config::pins::external_expander::REMOTE_INPUT_1
};

Input remote_input_2(
    remote_input_2_pin,
    true,    // active low
    50       // debounce ms
);

// ---------- Remote Input 2 Logic --------------
// Runs every loop AFTER all inputs have been updated by InputManager.
static void remote_input_2_button_tick(uint32_t now_ms)
{
    if (remote_input_2.released())
    {
        LOG("Winking LH Pop-up - Remote");
        LH_POP_UP.wink_pop_up();
    }
}

void remote_input_2_button_register()
{
    // Register the input so it gets update() called exactly once per loop.
    inputs_manager.add(remote_input_2);

    // Register this module's logic to run after updates.
    inputs_manager.add_task(remote_input_2_button_tick);
}