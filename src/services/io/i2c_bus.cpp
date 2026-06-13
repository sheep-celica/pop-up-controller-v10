#include "services/io/i2c_bus.h"

#include <Wire.h>
#include "config.h"

void setup_i2c_bus()
{
    Wire.begin(config::pins::i2c::SDA, config::pins::i2c::SCL);
    Wire.setTimeOut(config::pins::i2c::TIMEOUT_MS);
    Wire.setClock(config::pins::i2c::FREQUENCY_HZ);
}
