#include "services/inputs/types/input_pin.h"
#include "services/inputs/inputs_manager.h"
#include "services/io/io_expanders.h"


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

        default:
            return false;
    }
}

bool read_internal_expander(IoExpanderPin expander_pin)
{   
    uint8_t integer = static_cast<uint8_t>(expander_pin);
    return internal_ads.digitalRead(integer);
}

bool read_external_expander(IoExpanderPin expander_pin)
{
    return true;
}
