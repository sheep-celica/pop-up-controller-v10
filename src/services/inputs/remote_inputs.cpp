#include "services/inputs/remote_inputs.h"
#include "services/inputs/inputs_manager.h"
#include "config.h"


// ---------- Remote Input 1 (External Exapnder GPIO) ----------
static InputPin remote_input_1_pin {
    .backend = InputBackend::EXTERNAL_EXPANDER,
    .expander_pin = config::pins::external_expander::REMOTE_INPUT_0
};

Input remote_input_1(
    remote_input_1_pin,
    true,    // active low
    50       // debounce ms
);

// ---------- Remote Input 2 (External Exapnder GPIO) ----------
static InputPin remote_input_2_pin {
    .backend = InputBackend::EXTERNAL_EXPANDER,
    .expander_pin = config::pins::external_expander::REMOTE_INPUT_1
};

Input remote_input_2(
    remote_input_2_pin,
    true,    // active low
    50       // debounce ms
);

// ---------- Remote Input 3 (External Exapnder GPIO) ----------
static InputPin remote_input_3_pin {
    .backend = InputBackend::EXTERNAL_EXPANDER,
    .expander_pin = config::pins::external_expander::REMOTE_INPUT_2
};

Input remote_input_3(
    remote_input_3_pin,
    true,    // active low
    50       // debounce ms
);

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

// Register inputs
void register_remote_inputs()
{
    inputs_manager.add(remote_input_1);
    inputs_manager.add(remote_input_2);
    inputs_manager.add(remote_input_3);
    inputs_manager.add(remote_input_4);
}