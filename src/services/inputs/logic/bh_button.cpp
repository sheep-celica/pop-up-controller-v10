#include "services/inputs/logic/bh_button.h"
#include "services/inputs/inputs_manager.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "config.h"


// ---------- RH Button (ESP32 GPIO) ----------
static InputPin bh_button_pin {
    .backend = InputBackend::ESP32_GPIO,
    .esp32_pin = config::pins::BH_BUTTON_PIN
};

Input bh_button(
    bh_button_pin,
    true,    // active low
    30       // debounce ms
);

// ---------- RH Button Logic --------------
// Runs every loop AFTER all inputs have been updated by InputManager.
static void bh_button_tick(uint32_t now_ms)
{
    if (bh_button.released())
    {
        statistics_manager.record_bh_button_press();
        LOG("Winking Both Pop-up");
        RH_POP_UP.wink_pop_up();
        LH_POP_UP.wink_pop_up();
    }
}

void bh_button_register()
{
    // Register the input so it gets update() called exactly once per loop.
    inputs_manager.add(bh_button);

    // Register this module's logic to run after updates.
    inputs_manager.add_task(bh_button_tick);
}
