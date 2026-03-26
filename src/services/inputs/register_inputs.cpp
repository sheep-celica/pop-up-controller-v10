#include "services/inputs/register_inputs.h"
#include "services/inputs/inputs_manager.h"
#include "services/inputs/remote_input_pins.h"
#include "services/io/io_expanders.h"
#include "services/logging/logging.h"

#include "services/inputs/logic/rh_button.h"
#include "services/inputs/logic/lh_button.h"
#include "services/inputs/logic/bh_button.h"
#include "services/inputs/logic/toggle_button.h"
#include "services/inputs/logic/sleepy_eye_button.h"
#include "services/inputs/logic/debug_button.h"
#include "services/inputs/logic/light_switch_up.h"
#include "services/inputs/logic/light_switch_hold.h"
#include "services/inputs/logic/remote_input_1.h"
#include "services/inputs/logic/remote_input_2.h"
#include "services/inputs/logic/remote_input_3.h"
#include "services/inputs/logic/remote_input_4.h"

namespace {
    bool s_remote_inputs_registered = false;

    void register_remote_inputs_once()
    {
        if (s_remote_inputs_registered || !is_external_expander_connected())
        {
            return;
        }

        setup_remote_input_pin_mapping();
        LOG("Registering remote inputs.");
        remote_input_1_button_register();
        remote_input_2_button_register();
        remote_input_3_button_register();
        remote_input_4_button_register();
        s_remote_inputs_registered = true;
        LOG("%d inputs and %d tasks have been registered.", inputs_manager.input_count(), inputs_manager.task_count());
    }
}

void register_inputs()
{
    // Reserve with headroom for near-term input/task additions.
    inputs_manager.reserve(14, 14);

    // Register regular inputs
    LOG("Registering regular inputs");
    rh_button_register();
    lh_button_register();
    bh_button_register();
    toggle_button_register();
    sleepy_eye_button_register();
    debug_button_register();
    light_switch_up_register();
    light_switch_hold_register();

    // Check if remote module is connected and register remote inputs
    if (is_external_expander_connected())
    {
        register_remote_inputs_once();
    }
    else
    {
        LOG("Skipping remote inputs. PCF expander not connected.");
        LOG("%d inputs and %d tasks have been registered.", inputs_manager.input_count(), inputs_manager.task_count());
    }
}

void update_remote_input_registration()
{
    register_remote_inputs_once();
}
