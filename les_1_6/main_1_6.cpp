#include <string>
#include <iostream>
#include <Arduino.h>

#define PIN_PHOTO_SENSOR 10

#define ADC_MAX 4095.0
#define VOLTAGE_REF 3.1

volatile int sensorValue;
volatile float calculated_voltage;
volatile float real_voltage;
volatile float difference_percent;


void setup() {
  Serial.begin(115200);
}

void loop() {
  std::cout << std::string(100, '-') << std::endl;
  sensorValue = analogRead(PIN_PHOTO_SENSOR);

  calculated_voltage = (sensorValue / ADC_MAX) * VOLTAGE_REF;
  Serial.print("Calculated Voltage: ");
  Serial.print(calculated_voltage);
  Serial.println(" V");

  real_voltage = analogReadMilliVolts(PIN_PHOTO_SENSOR) / 1000.0; // millivolts to volts
  Serial.print("Real Voltage: ");
  Serial.print(real_voltage);
  Serial.println(" V");

  if (real_voltage != 0.0) {
    difference_percent = ((calculated_voltage - real_voltage) / real_voltage) * 100.0;
  } else {
    difference_percent = 0.0;
  }
  
  Serial.print("Difference: ");
  Serial.print(difference_percent);
  Serial.println(" %");

  delay(100);

}