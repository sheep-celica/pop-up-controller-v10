#include "services/inputs/remote_input_actions.h"

#include "services/diagnostics/diagnostics.h"
#include "services/inputs/logic/light_switch_up.h"
#include "services/inputs/remote_input_pins.h"
#include "services/io/remote_module.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"

namespace {
    uint8_t logical_input_for_physical_button(uint8_t physical_button)
    {
        const RemoteInputPinMapping mapping = get_remote_input_pin_mapping();
        const uint8_t physical_buttons[] = {
            mapping.remote_input_1,
            mapping.remote_input_2,
            mapping.remote_input_3,
            mapping.remote_input_4
        };

        for (uint8_t logical_input = 0; logical_input < 4; ++logical_input) {
            if (physical_buttons[logical_input] == physical_button) {
                return static_cast<uint8_t>(logical_input + 1u);
            }
        }
        return 0;
    }
}

void trigger_remote_input_action(uint8_t logical_input)
{
    if (diagnostics_active()) return;
    if (!is_light_switch_safely_off() && !are_remote_inputs_with_headlights_allowed()) return;

    switch (logical_input) {
        case 1:
            if (RH_POP_UP.get_target() != PopUpState::IDLE) break;
            statistics_manager.record_remote_input_press(1);
            LOG("Winking RH Pop-up - Remote");
            RH_POP_UP.wink_pop_up();
            break;
        case 2:
            if (LH_POP_UP.get_target() != PopUpState::IDLE) break;
            statistics_manager.record_remote_input_press(2);
            LOG("Winking LH Pop-up - Remote");
            LH_POP_UP.wink_pop_up();
            break;
        case 3:
        {
            const bool wink_rh = RH_POP_UP.get_target() == PopUpState::IDLE;
            const bool wink_lh = LH_POP_UP.get_target() == PopUpState::IDLE;
            if (!wink_rh && !wink_lh) break;
            statistics_manager.record_remote_input_press(3);
            LOG("Winking Both Pop-ups - Remote");
            if (wink_rh) RH_POP_UP.wink_pop_up();
            if (wink_lh) LH_POP_UP.wink_pop_up();
            break;
        }
        case 4:
            LOG("Remote input 4 pressed");
            if (toggle_sleepy_eye_mode()) statistics_manager.record_remote_input_press(4);
            break;
        default:
            break;
    }
}

void update_remote_module_inputs()
{
    uint8_t physical_button = 0;
    while (consume_remote_module_button_press(physical_button)) {
        trigger_remote_input_action(logical_input_for_physical_button(physical_button));
    }
}
