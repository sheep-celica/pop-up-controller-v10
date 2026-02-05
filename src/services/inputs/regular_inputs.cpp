#include "services/inputs/regular_inputs.h"
#include "services/inputs/inputs_manager.h"
#include "services/logging.h"
#include "services/pop_up_control.h"
#include "config.h"


// ---------- Light Switch HEAD (ESP32 GPIO) ----------
static InputPin light_switch_head_pin {
    .backend = InputBackend::INTERNAL_EXPANDER,
    .expander_pin = config::pins::internal_expander::LIGHT_SWITCH_UP_PIN
};

Input light_switch_head(
    light_switch_head_pin,
    true,    // active low
    50       // debounce ms
);

// ---------- Light Switch HOLD (ESP32 GPIO) ----------
static InputPin light_switch_hold_pin {
    .backend = InputBackend::INTERNAL_EXPANDER,
    .expander_pin = config::pins::internal_expander::LIGHT_SWITCH_HOLD_PIN
};

Input light_switch_hold(
    light_switch_hold_pin,
    true,    // active low
    50       // debounce ms
);


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

// ---------- BH Button (ESP32 GPIO) ----------
static InputPin bh_button_pin {
    .backend = InputBackend::ESP32_GPIO,
    .esp32_pin = config::pins::BH_BUTTON_PIN
};

Input bh_button(
    bh_button_pin,
    true,
    30
);

// ---------- Sleepy Eye Button (ESP32 GPIO) ----------
static InputPin sleepy_eye_button_pin {
    .backend = InputBackend::ESP32_GPIO,
    .esp32_pin = config::pins::SLEEPY_EYE_BUTTON_PIN
};

Input sleepy_eye_button(
    sleepy_eye_button_pin,
    true,
    60
);

// ---------- Debug Button (Internal Expander) ----------
static InputPin debug_button_pin {
    .backend = InputBackend::INTERNAL_EXPANDER,
    .expander_pin = config::pins::internal_expander::DEBUG_BUTTON_PIN
};

Input debug_button(
    debug_button_pin,
    true,
    50
);

// Register inputs
void register_regular_inputs()
{   
    LOG("Registering regular inputs.");
    pinMode(rh_button_pin.esp32_pin, INPUT_PULLUP);
    pinMode(lh_button_pin.esp32_pin, INPUT_PULLUP);
    pinMode(bh_button_pin.esp32_pin, INPUT_PULLUP);
    pinMode(sleepy_eye_button_pin.esp32_pin, INPUT_PULLUP);

    inputs_manager.add(light_switch_head);
    inputs_manager.add(light_switch_hold);
    inputs_manager.add(rh_button);
    inputs_manager.add(lh_button);
    inputs_manager.add(bh_button);
    inputs_manager.add(sleepy_eye_button);
    inputs_manager.add(debug_button);
}

void handle_regular_inputs()
{

    if (light_switch_head.is_high() && light_switch_head.get_stable_state_time() > 100)
    {
        if (RH_POP_UP.get_previous_target() != PopUpState::UP && RH_POP_UP.get_target() == PopUpState::IDLE)
        {
            LOG("RH Going UP");
            RH_POP_UP.set_target(PopUpState::UP);
        }
        if (LH_POP_UP.get_previous_target() != PopUpState::UP && LH_POP_UP.get_target() == PopUpState::IDLE)
        {
            LOG("LH Going UP");
            LH_POP_UP.set_target(PopUpState::UP);
        }
    }
    
    if (light_switch_head.is_low() && light_switch_hold.is_high() && light_switch_hold.get_stable_state_time() > 100)
    {
        if (RH_POP_UP.get_previous_target() != PopUpState::DOWN && RH_POP_UP.get_target() == PopUpState::IDLE)
        {
            LOG("RH Going DOWN");
            RH_POP_UP.set_target(PopUpState::DOWN);
        }
        if (LH_POP_UP.get_previous_target() != PopUpState::DOWN && LH_POP_UP.get_target() == PopUpState::IDLE)
        {
            LOG("LH Going DOWN");
            LH_POP_UP.set_target(PopUpState::DOWN);
        }
    }

    if (debug_button.pressed())
    {
        LOG("Debug Button has been pressed");
    }
    if (debug_button.released())
    {
        LOG("Debug Button has been released"); 
    }

    if (light_switch_head.pressed())
    {
        LOG("LIGHT-SWITCH HEAD has been pressed");
    }
    if (light_switch_head.released())
    {
        LOG("LIGHT-SWITCH HEAD has been released"); 
    }

    if (light_switch_hold.pressed())
    {
        LOG("LIGHT-SWITCH HOLD has been pressed");
    }
    if (light_switch_hold.released())
    {
        LOG("LIGHT-SWITCH HOLD has been released"); 
    }


    if (rh_button.pressed())
    {
        LOG("RH Button has been pressed");
    }
    if (rh_button.released())
    {
        LOG("RH Button has been released");
        RH_POP_UP.wink_pop_up(); 
    }

    if (lh_button.pressed())
    {
        LOG("LH Button has been pressed");
    }
    if (lh_button.released())
    {
        LOG("LH Button has been released");
        LH_POP_UP.wink_pop_up(); 
    }

    if (bh_button.pressed())
    {
        LOG("BH Button has been pressed");
    }
    if (bh_button.released())
    {
        LOG("BH Button has been released");
        RH_POP_UP.wink_pop_up();
        LH_POP_UP.wink_pop_up(); 
    }
    
    if (sleepy_eye_button.pressed())
    {
        LOG("Sleepy eye Button has been pressed");
    }
    if (sleepy_eye_button.released())
    {
        LOG("Sleepy eye Button has been released"); 
    }
}