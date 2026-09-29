#include <Arduino.h>

const uint8_t buttonPin = 18;
const uint32_t DEBOUNCE_TRASHOLD_US = 50000;

volatile bool is_button_pressed = false;
volatile uint32_t raw_interupt_counter = 0;

// Interrupt Service Routine (ISR)
void IRAM_ATTR button_ISR() {
    is_button_pressed = true;
    raw_interupt_counter++;
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
        Serial.print("Button pressed count: ");
        Serial.print(raw_interupt_counter);
        Serial.println(" times.");
        is_button_pressed = false;
    }
}