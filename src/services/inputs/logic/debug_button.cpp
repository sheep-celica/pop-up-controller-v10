#include "services/inputs/logic/debug_button.h"
#include "services/inputs/inputs_manager.h"
#include "services/io/leds.h"
#include "services/io/power.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "config.h"


// ---------- Debug Button (Internal Expander GPIO) ----------
static InputPin debug_button_pin {
    .backend = InputBackend::INTERNAL_EXPANDER,
    .expander_pin = config::pins::internal_expander::DEBUG_BUTTON_PIN
};

Input debug_button(
    debug_button_pin,
    true,    // active low
    30       // debounce ms
);

namespace {
    constexpr uint32_t k_press_window_ms = 1500;
    constexpr uint32_t k_long_press_ms = 5000;
    constexpr float k_status_blink_hz = 6.0f;

    uint32_t s_press_count_in_window = 0;
    uint32_t s_last_press_ms = 0;
    bool s_window_active = false;
    bool s_long_press_reported = false;
}

// ---------- Debug Button Logic --------------
// Runs every loop AFTER all inputs have been updated by InputManager.
static void debug_button_tick(uint32_t now_ms)
{
    if (s_window_active && (now_ms - s_last_press_ms) >= k_press_window_ms)
    {
        const uint32_t finalized_press_count = s_press_count_in_window;
        s_window_active = false;
        s_press_count_in_window = 0;

        if (finalized_press_count == 3)
        {
            const bool current_allow = is_sleepy_eye_mode_with_headlights_allowed();
            const bool next_allow = !current_allow;
            if (set_sleepy_eye_mode_with_headlights_allowed(next_allow))
            {
                LOG(
                    "Debug button: 3 presses -> ALLOW_SLEEPY_EYE_MODE_WITH_HEADLIGHTS set to %s.",
                    next_allow ? "TRUE" : "FALSE");
            }
            else
            {
                LOG("Debug button: 3 presses detected but failed to persist ALLOW_SLEEPY_EYE_MODE_WITH_HEADLIGHTS.");
            }
        }
        else if (finalized_press_count == 5)
        {
            LOG("Debug button: detected 5 presses within %lu ms.",
                static_cast<unsigned long>(k_press_window_ms));
        }
    }

    if (debug_button.is_low())
    {
        s_long_press_reported = false;
    }
    else if (!s_long_press_reported && debug_button.get_stable_state_time() >= k_long_press_ms)
    {
        s_long_press_reported = true;
        LOG("Debug button: held for at least %lu ms. Powering off and restarting.",
            static_cast<unsigned long>(k_long_press_ms));
        reboot_controller();
    }

    if (debug_button.pressed())
    {
        if (!s_window_active)
        {
            s_window_active = true;
            s_press_count_in_window = 0;
        }

        ++s_press_count_in_window;
        s_last_press_ms = now_ms;

        blink_led(LedId::STATUS_LED, 1, k_status_blink_hz);
    }
}

void debug_button_register()
{
    // Register the input so it gets update() called exactly once per loop.
    inputs_manager.add(debug_button);

    // Register this module's logic to run after updates.
    inputs_manager.add_task(debug_button_tick);
}
