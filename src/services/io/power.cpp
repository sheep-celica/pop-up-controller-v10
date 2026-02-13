#include "services/io/power.h"
#include "services/logging/logging.h"
#include "config.h"


void setup_power()
{
    pinMode(config::pins::POWER_ON_PIN, OUTPUT);
}

void power_on()
{
    LOG("Latching power ON.");
    digitalWrite(config::pins::POWER_ON_PIN, HIGH);
}

void power_off()
{
    LOG("Latching power OFF.");
    digitalWrite(config::pins::POWER_ON_PIN, LOW);
}

void check_idle_time()
{
    // TODO check idle time and POWER off if over spec
    if (false)
    {
        power_off();
    }
}