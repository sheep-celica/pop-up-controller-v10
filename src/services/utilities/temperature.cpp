#include "services/utilities/temperature.h"
#include "config.h"
#include "services/logging/logging.h"
#include <Wire.h>


Generic_LM75 temperature_sensor(config::utilities::STLM75_ADDRESS);

void setup_temperature()
{
    if (is_temperature_sensor_connected())
    {
        LOG("Temperature: %.2f C", read_temperature());
        return;
    }

    LOG("Temperature sensor not connected.");
    report_error_code(ErrorCode::TEMP_SENSOR_MALFUNCTION);
}

bool is_temperature_sensor_connected()
{
    Wire.beginTransmission(config::utilities::STLM75_ADDRESS);
    return Wire.endTransmission(true) == 0;
}

float read_temperature()
{
    const float measured = temperature_sensor.readTemperatureC();
    return measured;
}

const char* read_temperature_char()
{
    static char buf[12]; // enough for "-125.00\0"

    if (!is_temperature_sensor_connected())
    {
        snprintf(buf, sizeof(buf), "Not Connected");
        return buf;
    }

    long centi = lroundf(read_temperature() * 100.0f);
    long abs_centi = labs(centi);

    snprintf(buf, sizeof(buf),
             (centi < 0) ? "-%ld.%02ld" : "%ld.%02ld",
             abs_centi / 100, abs_centi % 100);

    return buf;
}
