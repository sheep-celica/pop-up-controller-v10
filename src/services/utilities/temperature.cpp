#include "services/utilities/temperature.h"
#include "config.h"


Generic_LM75 temperature_sensor(config::utilities::STLM75_ADDRESS);


float read_temperature()
{
    const float measured = temperature_sensor.readTemperatureC();
    return measured;
}

const char* read_temperature_char()
{
    static char buf[12]; // enough for "-125.00\0"
    long centi = lroundf(read_temperature() * 100.0f);
    long abs_centi = labs(centi);

    snprintf(buf, sizeof(buf),
             (centi < 0) ? "-%ld.%02ld" : "%ld.%02ld",
             abs_centi / 100, abs_centi % 100);

    return buf;
}
