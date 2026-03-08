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
    if (remote_pcf.isConnected())
    {
        setup_remote_input_pin_mapping();
        LOG("Registering remote inputs.");
        remote_input_1_button_register();
        remote_input_2_button_register();
        remote_input_3_button_register();
        remote_input_4_button_register();
    }
    else
    {
        LOG("Skipping remote inputs. PCF expander not connected.");
    }
    LOG("%d inputs and %d tasks have been registered.", inputs_manager.input_count(), inputs_manager.task_count());
}
