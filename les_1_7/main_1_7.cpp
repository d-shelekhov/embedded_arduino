#include <array>
#include <string>
#include <iostream>
#include <Arduino.h>

constexpr int PIN_PHOTO_SENSOR = 10;
constexpr int PIN_LED_RED = 17;
constexpr int PIN_LED_GREEN = 16;
constexpr int PIN_LED_YELLOW = 15;
constexpr int PIN_EXTERNAL_BUTTON = 18;
constexpr int PIN_POTENTIOMETER = 7;

constexpr unsigned long BLINK_ON_MS = 500;
constexpr unsigned long BLINK_OFF_MS = 500;

constexpr float ADC_MAX = 4095.0;
constexpr int LED_MIN_TRESHHOLD = 20;


enum class TrafficLightState {
    RED = 0,
    RED_YELLOW = 1,
    GREEN = 2,
    YELLOW = 3,
    BLINK_YELLOW = 4,
};

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(PIN_EXTERNAL_BUTTON, INPUT);
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_YELLOW, OUTPUT);
}


int getIlluminanceLevel(int sensorValue){
    int ledLevel = map(sensorValue, 0, 4095, 0, 255);

    if (ledLevel < LED_MIN_TRESHHOLD) {
        ledLevel = LED_MIN_TRESHHOLD;
    }
    return ledLevel;
}


class Potentiometer {
  private:
    int pin;

  public:
    Potentiometer(int pin) : pin(pin) {
        pinMode(pin, INPUT);
    }

    int readRaw() {
        return analogRead(pin);
    }

    int readPercent() {
        int raw = analogRead(pin);
        return map(raw, 0, 4095, 0, 100);
    }

    int readDigit() {
        return map(analogRead(pin), 0, 4095, 0, 10);
    }
};


class PasswordLock {

    private:
        static constexpr int PASSWORD_LENGTH = 4;

        std::array<int, PASSWORD_LENGTH> correctPassword = {1, 4, 7, 2};
        std::array<int, PASSWORD_LENGTH> enteredDigits{};

        int enteredCount = 0;
        int lastButtonState = LOW;
        int lastDisplayedDigit = -1;

        bool unlocked = false;
        bool introPrinted = false;   // so the "waiting for input" message doesn't spam every loop() iteration
    
        bool checkPassword() {
            return enteredDigits == correctPassword;
        }
    
    public:
        bool isUnlocked() const {
            return unlocked;
        }

        void run(int currentDigit, int buttonState) {
            if (unlocked) {
                return;
            }

            if (!introPrinted) {
                std::cout << "[PASSWORD] Traffic Light is locked. "
                << "Waiting for password input. Turn the potentiometer, press the button to confirm a digit (0-10).\n";
                introPrinted = true;
            }

            if (currentDigit != lastDisplayedDigit) {
                std::cout << "\r[PASSWORD] current digit: " << currentDigit << "   " << std::flush;
                lastDisplayedDigit = currentDigit;
            }

            if (buttonState == HIGH && lastButtonState == LOW) {
                enteredDigits[enteredCount] = currentDigit;
                enteredCount++;

                std::cout << "[PASSWORD] digit " << enteredCount << "/" << PASSWORD_LENGTH << " accepted: " << currentDigit << "\n";

                if (enteredCount == PASSWORD_LENGTH) {
                    if (checkPassword()) {
                        unlocked = true;
                        std::cout << "[PASSWORD] correct! Unlocked.\n";
                    } else {
                        std::cout << "[PASSWORD] incorrect, try again.\n";
                    }
                    enteredCount = 0; 
                }
            }
            lastButtonState = buttonState;
        }
    };


class TrafficLight {

    private:
        TrafficLightState currentState;
        unsigned long phaseStartTime = 0;

        bool nightMode = false;
        bool blinkOn = false;
        unsigned long lastBlinkToggle = 0;
        int lastExternalButtonState = LOW;

        void runRegularMode() {
            switch (currentState) {
                case TrafficLightState::RED:
                    std::cout << "[RED] Stop! Waiting for 5 seconds...\n";
                    currentState = TrafficLightState::RED_YELLOW;
                    break;
                case TrafficLightState::RED_YELLOW:
                    std::cout << "[GREEN] Be ready! Driving for 2 seconds...\n";
                    currentState = TrafficLightState::GREEN;
                    break;
                case TrafficLightState::GREEN:
                    std::cout << "[GREEN] Go! Driving for 4 seconds...\n";
                    currentState = TrafficLightState::YELLOW;
                    break;
                case TrafficLightState::YELLOW:
                    std::cout << "[YELLOW] Caution! Slowing down for 2 seconds...\n";
                    currentState = TrafficLightState::RED;
                    break;
            }
        }
    
        void toggleNightMode() {
            nightMode = !nightMode;
            blinkOn = false;
            lastBlinkToggle = millis();
            phaseStartTime = millis();
        }

        void turnOffAllLights() {
            analogWrite(PIN_LED_RED, 0);
            analogWrite(PIN_LED_GREEN, 0);
            analogWrite(PIN_LED_YELLOW, 0);
        }
    
        void setLight(TrafficLightState state, int ledLevel) {
            turnOffAllLights();

            switch (state) {
                case TrafficLightState::RED:
                    analogWrite(PIN_LED_RED, ledLevel);
                    break;
                case TrafficLightState::RED_YELLOW:
                    analogWrite(PIN_LED_RED, ledLevel);
                    analogWrite(PIN_LED_YELLOW, ledLevel);
                    break;
                case TrafficLightState::GREEN:
                    analogWrite(PIN_LED_GREEN, ledLevel);
                    break;
                case TrafficLightState::YELLOW:
                    analogWrite(PIN_LED_YELLOW, ledLevel);
                    break;
                case TrafficLightState::BLINK_YELLOW:
                    analogWrite(PIN_LED_YELLOW, ledLevel);
                    break;
            }
        }

        unsigned long getPhaseDuration(TrafficLightState state) {
            switch (state) {
                case TrafficLightState::RED:        return 5000;
                case TrafficLightState::RED_YELLOW: return 1500;
                case TrafficLightState::GREEN:      return 5000;
                case TrafficLightState::YELLOW:     return 2000;
                default:                            return 0; 
            }
        }

    public:
        TrafficLight() : currentState(TrafficLightState::RED) {}

        void run(int illuminanceLevel, int externalButtonState) {

            if (externalButtonState == HIGH && lastExternalButtonState == LOW) {
                toggleNightMode();
            }
            lastExternalButtonState = externalButtonState;
            unsigned long now = millis();

            if (nightMode) {
                unsigned long phase = blinkOn ? BLINK_ON_MS : BLINK_OFF_MS;

                if (now - lastBlinkToggle >= phase) {
                    blinkOn = !blinkOn;
                    lastBlinkToggle = now;

                    setLight(TrafficLightState::YELLOW, blinkOn ? illuminanceLevel : 0);
                }
                return;
            }

            if (now - phaseStartTime >= getPhaseDuration(currentState)) {
                runRegularMode();
                phaseStartTime = now;
            }
            setLight(currentState, illuminanceLevel);
        }
        
};

PasswordLock passwordLock;
Potentiometer potentiometer(PIN_POTENTIOMETER);
TrafficLight trafficLight;

int main() {
    while (true){
        int sensorValue = analogRead(PIN_PHOTO_SENSOR);
        int buttonState = digitalRead(PIN_EXTERNAL_BUTTON);
        int digit = potentiometer.readDigit();

        if (!passwordLock.isUnlocked()) {
            passwordLock.run(digit, buttonState);
        } else {
            int illuminanceLevel = getIlluminanceLevel(sensorValue);
            trafficLight.run(illuminanceLevel, buttonState);
        }
        delay(20);
    }
}
