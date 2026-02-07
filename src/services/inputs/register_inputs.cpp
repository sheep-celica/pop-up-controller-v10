#include "services/inputs/register_inputs.h"
#include "services/io/io_expanders.h"
#include "services/logging.h"

#include "services/inputs/logic/rh_button.h"
#include "services/inputs/logic/lh_button.h"
#include "services/inputs/logic/bh_button.h"
#include "services/inputs/logic/sleepy_eye_button.h"
#include "services/inputs/logic/remote_input_1.h"
#include "services/inputs/logic/remote_input_2.h"
#include "services/inputs/logic/remote_input_3.h"
#include "services/inputs/logic/remote_input_4.h"



void register_inputs()
{
    // Register regular inputs
    LOG("Registering regular inputs");
    rh_button_register();
    lh_button_register();
    bh_button_register();
    sleepy_eye_button_register();

    // Check if remote module is connected and register remote inputs
    if (remote_pcf.isConnected())
    {
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
}
