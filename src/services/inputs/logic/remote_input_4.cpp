#include "services/inputs/logic/remote_input_4.h"
#include "services/inputs/inputs_manager.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "services/io/leds.h"
#include "config.h"


// ---------- Remote Input 4 (External Exapnder GPIO) ----------
static InputPin remote_input_4_pin {
    .backend = InputBackend::EXTERNAL_EXPANDER,
    .expander_pin = config::pins::external_expander::REMOTE_INPUT_3
};

Input remote_input_4(
    remote_input_4_pin,
    true,    // active low
    50       // debounce ms
);

// ---------- Remote Input 4 Logic --------------
// Runs every loop AFTER all inputs have been updated by InputManager.
static void remote_input_4_button_tick(uint32_t now_ms)
{
    if (remote_input_4.released())
    {
        statistics_manager.record_remote_input_press(4);
        LOG("Toggling sleepy eye mode - Remote");
        bool sleepy_eye_mode_on = !RH_POP_UP.get_sleepy_eye_mode();
        LOG("Toggling sleepy eye mode to %d", sleepy_eye_mode_on);
        RH_POP_UP.set_sleepy_eye_mode(sleepy_eye_mode_on);
        LH_POP_UP.set_sleepy_eye_mode(sleepy_eye_mode_on);
        set_led_state(LedId::SLEEPY_EYE_STATUS, sleepy_eye_mode_on);
    }
}

void remote_input_4_button_register()
{
    // Register the input so it gets update() called exactly once per loop.
    inputs_manager.add(remote_input_4);

    // Register this module's logic to run after updates.
    inputs_manager.add_task(remote_input_4_button_tick);
}
