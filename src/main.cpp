// #include <Arduino.h>
// #include "ADS7138.h"
// #include <Wire.h>


#define RH_MOTOR 23
#define LH_MOTOR 19
#define RH_BRAKE 18  
#define LH_BRAKE 17

#include <Wire.h>
#include <PCF8574.h>
#include "ADS7138.h"

static const uint8_t PCF_ADDR = 0x24;
bool led_on = false;

ADS7138 adc(0x10, 3.3f);
PCF8574 pcf(PCF_ADDR);


void setup() {
  Serial.begin(115200);
  Wire.begin();




  Serial.print("\nTesting out BTS6163D.");
  pinMode(RH_MOTOR, OUTPUT);
  pinMode(LH_MOTOR, OUTPUT);
  pinMode(RH_BRAKE, OUTPUT);
  pinMode(LH_BRAKE, OUTPUT);

  analogWrite(RH_BRAKE, 0);
  analogWrite(LH_BRAKE, 0);
  digitalWrite(RH_MOTOR, LOW);
  digitalWrite(LH_MOTOR, LOW);

  // Start the expander
  // Serial.print("\nCycling states...");
  // delay(200);
  // digitalWrite(RH_MOTOR, HIGH);
  // digitalWrite(LH_MOTOR, LOW);
  // delay(500);
  // digitalWrite(RH_MOTOR, LOW);
  // digitalWrite(LH_MOTOR, HIGH);
  // delay(500);
  // digitalWrite(RH_MOTOR, HIGH);
  // digitalWrite(LH_MOTOR, HIGH);
  // delay(500);
  // digitalWrite(RH_MOTOR, LOW);
  // digitalWrite(LH_MOTOR, LOW);
  // // delay(200);
  // Serial.print("\nSetting up ADC.");
  // adc.setAnalogInput(0);
  // adc.setAnalogInput(2);
  // adc.setAnalogInput(3);

  // adc.setDigitalOutput(5, true);
  // adc.setDigitalOutput(6, true);
  // adc.setDigitalOutput(7, true);

  // adc.setDigitalInput(4);

  // adc.setOversampling(ADS7138::Oversampling::OSR_32);

  // if (!adc.begin(Wire)) {
  //   Serial.println("ADS7138 begin failed!");
  //   while (1) delay(100);
  // }

  // // Serial.print("\nChecking for debug button.");
  // // while (true)
  // // {
  // //   bool state = adc.digitalRead(4);
  // //   Serial.println(state ? "HIGH" : "LOW");
  // //   delay(333);
  // // }


  // Serial.print("\nActivating brakes");
  // while (true)
  // {
  //   for (int i=0; i<255; i++)
  //   {
  //     analogWrite(RH_BRAKE, i);
  //     analogWrite(LH_BRAKE, i);
  //     delay(10);
  //   }
  //   for (int i=0; i<255; i++)
  //   {
  //     analogWrite(RH_BRAKE, 255-i);
  //     analogWrite(LH_BRAKE, 255-i);
  //     delay(10);
  //   }
  // }
}

void loop() {
  bool p0 = pcf.read(0);
  bool p1 = pcf.read(1);
  bool p2 = pcf.read(2);
  bool p3 = pcf.read(3);

  Serial.printf("P0=%d P1=%d P2=%d P3=%d\n", p0, p1, p2, p3);

  delay(200);

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
