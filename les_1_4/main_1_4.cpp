#include <Arduino.h>
#include <iostream>

#define PIN_LED_GREEN (15)
#define PIN_LED_RED (16)
#define PIN_EXTERNAL_BUTTON (18)
#define BOOT_BUTTON_PIN (0)

enum Modes {
    SYNC = 0,
    RAILWAY = 1,
    OFF = 2,
};

int currentMode = Modes::OFF;

int lastExternalButtonState = LOW;   // idle LOW, pressed HIGH
int lastInternalButtonState = HIGH;  // idle BOOT = HIGH, pressed LOW

bool ledPhase = false;

void setup() {
    Serial.begin(115200);
    Serial.println("Started");
    delay(1000);

    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);

    pinMode(PIN_EXTERNAL_BUTTON, INPUT);
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
}

void process_external_button(int external_button_state) {
    if (external_button_state == HIGH && lastExternalButtonState == LOW) {
        currentMode = Modes::SYNC;
        Serial.println("External button. Sync mode");
    }
    lastExternalButtonState = external_button_state;
}

void process_internal_button(int internal_button_state) {
    if (internal_button_state == LOW && lastInternalButtonState == HIGH) {
        currentMode = Modes::RAILWAY;
        Serial.println("Boot button. Railway mode");
    }
    lastInternalButtonState = internal_button_state;
}

void loop() {
    int external_button_state = digitalRead(PIN_EXTERNAL_BUTTON);
    int internal_button_state = digitalRead(BOOT_BUTTON_PIN);

    process_external_button(external_button_state);
    process_internal_button(internal_button_state);

    ledPhase = !ledPhase;

    switch (currentMode){
        case Modes::RAILWAY: {
            digitalWrite(PIN_LED_GREEN, ledPhase);
            digitalWrite(PIN_LED_RED, !ledPhase);
            delay(1000);
            break;
        } 
        case Modes::SYNC: {        
            digitalWrite(PIN_LED_GREEN, ledPhase);
            digitalWrite(PIN_LED_RED, ledPhase);
            delay(200);
            break;
        }
        case Modes::OFF: {
            digitalWrite(PIN_LED_GREEN, LOW);
            digitalWrite(PIN_LED_RED, LOW);
            break;
        }
        default: std::cout << "Error"; break;
    }
}
