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
#include "services/utilities/controller_status.h"
#include "services/utilities/utilities.h"
#include "services/io/io_expanders.h"
#include "services/io/power.h"
#include "services/io/leds.h"
#include "services/commands/commands.h"


#define BUILD_VERSION "1.0.13"
#define BUILD_TIMESTAMP "2026-03-19T17:31:22Z"

namespace {
  bool s_bench_mode = false;
  bool s_bench_led_on = false;
  uint32_t s_bench_led_last_toggle_ms = 0;
  constexpr uint32_t kBenchLedTogglePeriodMs = 100;

  bool should_enter_bench_mode(float battery_voltage)
  {
    return battery_voltage < config::utilities::BENCH_MODE_MAX_BATTERY_V;
  }

  void update_bench_mode_led_indicator(uint32_t now_ms)
  {
    if (!s_bench_mode)
    {
      return;
    }

    if ((now_ms - s_bench_led_last_toggle_ms) < kBenchLedTogglePeriodMs)
    {
      return;
    }

    s_bench_led_last_toggle_ms = now_ms;
    s_bench_led_on = !s_bench_led_on;
    set_led_state(LedId::STATUS_LED, s_bench_led_on);
    set_led_state(LedId::INPUT_LED, s_bench_led_on);
  }
}

void setup()
{
  setup_power();

  // Setup functions
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(config::pins::i2c::FREQUENCY_HZ);
  setup_io_expanders();
  setup_pop_ups();
  register_inputs();
  initialize_logging(BUILD_VERSION, BUILD_TIMESTAMP);
  setup_leds();

  // Information print out
  LOG("Pop-up Controller V10 firmware loaded.");
  LOG("Build version: %s", BUILD_VERSION);
  LOG("Build timestamp: %s", BUILD_TIMESTAMP);
  const float battery_voltage = read_battery_voltage();
  const float temperature = read_temperature();
  s_bench_mode = should_enter_bench_mode(battery_voltage);
  set_controller_bench_mode_enabled(s_bench_mode);
  s_bench_led_last_toggle_ms = millis();
  s_bench_led_on = false;
  LOG("Temperature: %.2f C", temperature);
  LOG("Battery voltage: %.2f V", battery_voltage);
  LOG("Bench mode: %s", s_bench_mode ? "ENABLED (pop-up updates disabled)" : "DISABLED");
}


void loop() 
{
  const uint32_t now_ms = millis();
  if (!s_bench_mode)
  {
    inputs_manager.update();
    update_pop_ups();
  }
  update_bench_mode_led_indicator(now_ms);
  update_leds();
  statistics_manager.update_runtime();
  update_commands();
  check_idle_time();
}
