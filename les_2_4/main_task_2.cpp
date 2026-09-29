#include <Arduino.h>

const uint8_t buttonPin = 18;
const uint32_t DEBOUNCE_TRASHOLD_US = 50000;

volatile bool is_button_pressed = false;
volatile uint32_t last_interupt_us = 0;
volatile uint32_t debounced_counter = 0; 
volatile uint32_t last_accepted_interupt_us = 0;

// Interrupt Service Routine (ISR)
void IRAM_ATTR button_ISR() {
    is_button_pressed = true;
    last_interupt_us = micros();
}

void setup() {
    Serial.begin(115200);
    delay(500);
    pinMode(buttonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonPin), button_ISR, FALLING);
    Serial.println("Waiting for FALLING interrupts on GPIO 18...");
}

void loop() {
  if (is_button_pressed) {
    is_button_pressed = false;
 
    if (last_interupt_us - last_accepted_interupt_us >= DEBOUNCE_TRASHOLD_US) {
        last_accepted_interupt_us = last_interupt_us;
        debounced_counter++;

        Serial.print("Button pressed count: ");
        Serial.print(debounced_counter);
        Serial.println(" times.");
    } 
  }
}