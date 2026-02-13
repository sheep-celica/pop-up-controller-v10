#include "services/inputs/logic/rh_button.h"
#include "services/inputs/inputs_manager.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "config.h"


// ---------- RH Button (ESP32 GPIO) ----------
static InputPin rh_button_pin {
    .backend = InputBackend::ESP32_GPIO,
    .esp32_pin = config::pins::RH_BUTTON_PIN
};

Input rh_button(
    rh_button_pin,
    true,    // active low
    30       // debounce ms
);

// ---------- RH Button Logic --------------
// Runs every loop AFTER all inputs have been updated by InputManager.
static void rh_button_tick(uint32_t now_ms)
{
    if (rh_button.released())
    {
        LOG("Winking RH Pop-up");
        RH_POP_UP.wink_pop_up();
    }
}

void rh_button_register()
{
    // Register the input so it gets update() called exactly once per loop.
    inputs_manager.add(rh_button);

    // Register this module's logic to run after updates.
    inputs_manager.add_task(rh_button_tick);
}