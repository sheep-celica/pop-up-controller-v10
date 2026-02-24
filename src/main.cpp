// #include <Arduino.h>
// #include "ADS7138.h"
// #include <Wire.h>

#include <Wire.h>
#include <PCF8574.h>
#include "helpers/ADS7138.h"
#include "helpers/motor_controller.h"
#include "services/pop_up_control/pop_up_control.h"
#include "services/logging/logging.h"
#include "services/inputs/inputs_manager.h"
#include "services/inputs/register_inputs.h"
#include "services/utilities/temperature.h"
#include "services/io/io_expanders.h"
#include "services/io/power.h"
#include "services/io/leds.h"
#include "services/commands/commands.h"


#define BUILD_VERSION "1.0.10"
#define BUILD_TIMESTAMP "2026-02-24T22:30:55Z"


void setup()
{
  // Setup functions
  Serial.begin(115200);
  Wire.begin();
  setup_io_expanders();
  setup_pop_ups();
  register_inputs();
  initialize_logging(BUILD_VERSION, BUILD_TIMESTAMP);  // Last, to avoid blocking I2C or other init
  setup_power();
  setup_leds();
  
  // Latching on power
  power_on();

  LOG("Pop-up Controller V10 firmware loaded.");
  LOG("Build version: %s", BUILD_VERSION);
  LOG("Build timestamp: %s", BUILD_TIMESTAMP);
  print_manufacture_data();
  statistics_manager.print_statistics();
}

bool temp_flag = false;

void loop() 
{
  inputs_manager.update();
  update_pop_ups();
  update_leds();
  statistics_manager.update_runtime();
  update_commands();
  // if (millis() > 2000 && !temp_flag)
  // {
  //   temp_flag = true;
  //   turn_off_illumination();
  // }
  // if (millis() > 4000)
  // {
  //   delay(120000);
  // }
}
