#include "services/inputs/logic/sleepy_eye_button.h"
#include "services/inputs/inputs_manager.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "config.h"


// ---------- Sleepy Eye Button (ESP32 GPIO) ----------
static InputPin sleepy_eye_button_pin {
    .backend = InputBackend::ESP32_GPIO,
    .esp32_pin = config::pins::SLEEPY_EYE_BUTTON_PIN
};

Input sleepy_eye_button(
    sleepy_eye_button_pin,
    true,    // active low
    30       // debounce ms
);

// ---------- Sleepy Eye Button Logic --------------
// Runs every loop AFTER all inputs have been updated by InputManager.
static void sleepy_eye_button_tick(uint32_t now_ms)
{
    (void)now_ms;

    if (sleepy_eye_button.released())
    {
        LOG("Sleepy eye button released");
        (void)toggle_sleepy_eye_mode();
    }

}

void sleepy_eye_button_register()
{
    // Register the input so it gets update() called exactly once per loop.
    inputs_manager.add(sleepy_eye_button);

    // Register this module's logic to run after updates.
    inputs_manager.add_task(sleepy_eye_button_tick);
}
