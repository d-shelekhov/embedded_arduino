#include <Arduino.h>
#include <iostream>


constexpr uint8_t CONTROL_PIN = 7;   // transistor base
constexpr uint8_t MEASURE_PIN = 8;   // transistor collector

void setup() {
  pinMode(CONTROL_PIN, OUTPUT);
  pinMode(MEASURE_PIN, INPUT);

  digitalWrite(CONTROL_PIN, LOW);  // off
  Serial.begin(115200);
  delay(500);
}

void loop() {
  int initial_state = digitalRead(MEASURE_PIN);

  unsigned long start_time = micros();
  digitalWrite(CONTROL_PIN, HIGH);

  while (digitalRead(MEASURE_PIN) == initial_state) {
    // waiting to switch
  }

  unsigned long end_time = micros();

  unsigned long execution_time = end_time - start_time;

  Serial.print("Execution time = ");
  Serial.print(execution_time);
  Serial.println(" mks");

  delay(2000);
  digitalWrite(CONTROL_PIN, LOW);   // off till the next iteration
  delay(2000);
}