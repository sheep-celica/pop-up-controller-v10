#include <Wire.h>
#include <PCF8574.h>
#include "config.h"
#include "helpers/ADS7138.h"
#include "helpers/motor_controller.h"
#include "services/pop_up_control/pop_up_control.h"
#include "services/logging/logging.h"
#include "services/inputs/inputs_manager.h"
#include "services/inputs/register_inputs.h"
#include "services/utilities/temperature.h"
#include "services/utilities/utilities.h"
#include "services/io/io_expanders.h"
#include "services/io/power.h"
#include "services/io/leds.h"
#include "services/commands/commands.h"


#define BUILD_VERSION "1.0.52"
#define BUILD_TIMESTAMP "2026-02-28T20:56:43Z"


void setup()
{
  // Setup functions
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(config::pins::i2c::FREQUENCY_HZ);
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
  error_log_manager.print_error_log_entries();
  LOG("Temperature: %.2f C", read_temperature());
  LOG("Battery voltage: %.2f V", read_battery_voltage());
}

unsigned long last_status_log_ms = 0;

void loop() 
{
  const unsigned long now = millis();

  inputs_manager.update();
  update_pop_ups();
  update_leds();
  statistics_manager.update_runtime();
  update_commands();
}
