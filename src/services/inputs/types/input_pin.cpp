#include "services/inputs/types/input_pin.h"
#include "services/inputs/inputs_manager.h"
#include "services/io/io_expanders.h"
#include "services/pop_up_control/pop_up_control.h"


// These functions are assumed to exist or will be implemented by you
bool read_internal_expander(IoExpanderPin pin);
bool read_external_expander(IoExpanderPin pin);

bool InputPin::read() const
{
    switch (backend)
    {
        case InputBackend::ESP32_GPIO:
            return digitalRead(esp32_pin);

        case InputBackend::INTERNAL_EXPANDER:
            return read_internal_expander(expander_pin);

        case InputBackend::EXTERNAL_EXPANDER:
            return read_external_expander(expander_pin);

        case InputBackend::FAULT_EXPANDER:
            return read_fault_expander_pin(expander_pin);

        default:
            return false;
    }
}

bool read_internal_expander(IoExpanderPin expander_pin)
{
    return read_local_expander_pin(expander_pin);
}

bool read_external_expander(IoExpanderPin expander_pin)
{
    // Treat unavailable remote inputs as electrically inactive so runtime
    // expander disconnects cannot keep the controller polling a missing PCF8574.
    if (!is_external_expander_connected())
    {
        return true;
    }

    return read_external_expander_pin(expander_pin);
}
