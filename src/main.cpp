// #include <Arduino.h>
// #include "ADS7138.h"
// #include <Wire.h>

#include <Wire.h>
#include <PCF8574.h>
#include "helpers/ADS7138.h"
#include "helpers/motor_controller.h"
#include "services/pop_up_control.h"
#include "services/logging.h"
#include "services/inputs/inputs_manager.h"
#include "services/inputs/register_inputs.h"


void setup()
{
  Serial.begin(115200);
  Wire.begin();
  setup_pop_ups();
  register_inputs();

}

void loop() 
{
  inputs_manager.update();

  // bool p0 = pcf.read(0);
  // bool p1 = pcf.read(1);
  // bool p2 = pcf.read(2);
  // bool p3 = pcf.read(3);

  // Serial.printf("P0=%d P1=%d P2=%d P3=%d\n", p0, p1, p2, p3);

  // delay(200);

  // float v2 = adc.readAnalogVolts(2);
  // float v3 = adc.readAnalogVolts(3);

  // float vpin = adc.readAnalogVolts(0);
  // float vbat = vpin * 6.0f; // 10k/2k divider

  // Serial.print("AIN2: "); Serial.print(v2, 3);
  // Serial.print("  AIN3: "); Serial.print(v3, 3);
  // Serial.print("  VBAT: "); Serial.print(vbat, 2);
  // Serial.println(" V");

  // adc.digitalWrite(5, led_on);
  // adc.digitalWrite(6, led_on);
  // adc.digitalWrite(7, led_on);
  // led_on = !led_on;
  // delay(1000);
}
