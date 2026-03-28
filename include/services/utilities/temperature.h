#pragma once

#include <Temperature_LM75_Derived.h>


void setup_temperature();
bool is_temperature_sensor_connected();
float read_temperature();
const char* read_temperature_char();
