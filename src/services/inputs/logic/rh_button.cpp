#include "services/inputs/logic/rh_button.h"
#include "services/inputs/inputs_manager.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "services/io/leds.h"
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
        statistics_manager.record_rh_button_press();
        // LOG("Winking RH Pop-up");
        // RH_POP_UP.wink_pop_up();
        
        bool sleepy_eye_mode_on = !RH_POP_UP.get_sleepy_eye_mode();
        LOG("Toggling sleepy eye mode to %d", sleepy_eye_mode_on);
        RH_POP_UP.set_sleepy_eye_mode(sleepy_eye_mode_on);
        LH_POP_UP.set_sleepy_eye_mode(sleepy_eye_mode_on);
        set_led_state(LedId::SLEEPY_EYE_STATUS, sleepy_eye_mode_on);
    }
}

void rh_button_register()
{
    // Register the input so it gets update() called exactly once per loop.
    inputs_manager.add(rh_button);

    // Register this module's logic to run after updates.
    inputs_manager.add_task(rh_button_tick);
}
