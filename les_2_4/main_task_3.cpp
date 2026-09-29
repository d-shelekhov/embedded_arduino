#include <Arduino.h>

// Global variables for the button
const uint8_t buttonPin = 18;
const uint32_t DEBOUNCE_TRASHOLD_US = 50000;

volatile bool is_button_pressed = false;
volatile uint32_t debounced_counter = 0; 


// Interrupt Service Routine (ISR)
void IRAM_ATTR buttonISR() {
    is_button_pressed = true;
}

void setup() {
    Serial.begin(115200);
    delay(500);
    pinMode(buttonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, FALLING);
    Serial.println("Waiting for FALLING interrupts on GPIO 18...");
}

void loop() {
  if (is_button_pressed) {
    if (digitalRead(buttonPin) == LOW) {
        debounced_counter++;
 
        Serial.print("Button pressed count: ");
        Serial.print(debounced_counter);
        Serial.println(" times.");
    }
    is_button_pressed = false;
  }
}