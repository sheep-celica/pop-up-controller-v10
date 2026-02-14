#include "services/inputs/logic/lh_button.h"
#include "services/inputs/inputs_manager.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "config.h"


// ---------- LH Button (ESP32 GPIO) ----------
static InputPin lh_button_pin {
    .backend = InputBackend::ESP32_GPIO,
    .esp32_pin = config::pins::LH_BUTTON_PIN
};

Input lh_button(
    lh_button_pin,
    true,
    30
);

// ---------- LH Button Logic --------------
// Runs every loop AFTER all inputs have been updated by InputManager.
static void lh_button_tick(uint32_t now_ms)
{
    if (lh_button.released())
    {
        LOG("Winking LH Pop-up");
        LH_POP_UP.wink_pop_up();
    }
}

void lh_button_register()
{
    // Register the input so it gets update() called exactly once per loop.
    inputs_manager.add(lh_button);

    // Register this module's logic to run after updates.
    inputs_manager.add_task(lh_button_tick);
}