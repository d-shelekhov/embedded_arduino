#include <Arduino.h>
#define PIN_LED_GREEN (18)
#define PIN_LED_RED (2)
#define PIN_LED_WHITE (42)

void setup() {
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_WHITE, OUTPUT);
}

void blink_red() {
  digitalWrite(PIN_LED_RED, HIGH);
  delay(1000);
  digitalWrite(PIN_LED_RED, LOW);
}

void blink_white() {
  digitalWrite(PIN_LED_WHITE, HIGH);
  delay(1000);
  digitalWrite(PIN_LED_WHITE, LOW);
}

void blink_green() {
  digitalWrite(PIN_LED_GREEN, HIGH);
  delay(1000);
  digitalWrite(PIN_LED_GREEN, LOW);
}

void loop() {
  blink_red();
  delay(500);
  blink_white();
  delay(500);
  blink_green();
  delay(500);
}
