#include <Arduino.h>
#include "soc/gpio_reg.h"   // GPIO_OUT_W1TS_REG / GPIO_OUT_W1TC_REG

constexpr int LED_RED_PIN    = 17;
constexpr int LED_YELLOW_PIN = 15;
constexpr int LED_GREEN_PIN  = 16;

// Display segments a, b, c, d, e, f, g (common anode: segment is on when its pin is LOW)
constexpr int DISPLAY_SEGMENT_PINS[7] = {4, 5, 6, 7, 8, 9, 10};

constexpr uint8_t DISPLAY_DIGIT_MASKS[10] = {
  0b0111111,  // 0
  0b0000110,  // 1
  0b1011011,  // 2
  0b1001111,  // 3
  0b1100110,  // 4
  0b1101101,  // 5
  0b1111101,  // 6
  0b0000111,  // 7
  0b1111111,  // 8
  0b1101111   // 9
};

// true = start in blinking-yellow mode (unregulated intersection)
constexpr bool UNREGULATED_MODE = false;

constexpr uint32_t DURATION_GREEN_MS      = 4000;
constexpr uint32_t DURATION_YELLOW_MS     = 2000;
constexpr uint32_t DURATION_RED_MS        = 5000;
constexpr uint32_t DURATION_RED_YELLOW_MS = 1500;
constexpr uint32_t BLINK_HALF_PERIOD_MS   = 300;
constexpr int      GREEN_BLINK_COUNT      = 3;

class CountdownDisplay {
public:
  void begin() {
    for (int pin : DISPLAY_SEGMENT_PINS) {
      pinMode(pin, OUTPUT);
    }
    drawDigit(-1);  // display off
  }

  void showDigit(int digit) {
    if (digit == shownDigit) {
      return;
    }
    shownDigit = digit;
    drawDigit(digit);
  }

private:
  int shownDigit = -1;

  // Common anode: the segment is on when its pin is LOW.
  void writeCommonAnodePin(int pin, bool on) {
    digitalWrite(pin, on ? LOW : HIGH);
  }

  void drawDigit(int digit) {
    uint8_t mask = (digit >= 0 && digit <= 9) ? DISPLAY_DIGIT_MASKS[digit] : 0;
    for (int i = 0; i < 7; i++) {
      bool isSegmentEnabled = (mask >> i) & 1;
      writeCommonAnodePin(DISPLAY_SEGMENT_PINS[i], isSegmentEnabled);
    }
  }
};


class TrafficLight {
public:
  void begin(bool unregulatedMode) {
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    startPhase(unregulatedMode ? BLINK_YELLOW : GREEN);
  }

  // Called from the timer ISR every 1 ms. Only counts down and sets a flag.
  void IRAM_ATTR tick() {
    portENTER_CRITICAL_ISR(&timerMux);
    if (displayPhaseLeftMs > 0) {
      displayPhaseLeftMs--;
    }
    if (timeLeftMs > 0 && --timeLeftMs == 0) {
      isPhaseEnded = true;
    }
    portEXIT_CRITICAL_ISR(&timerMux);
  }

  // Called from loop(): if the ISR has set the flag, move to the next step.
  void update() {
    portENTER_CRITICAL(&timerMux);
    bool phaseEnded = isPhaseEnded;
    isPhaseEnded = false;
    portEXIT_CRITICAL(&timerMux);

    if (phaseEnded) {
      handlePhaseEnd();
    }
  }

  // Time until the color changes, for the display (0 = nothing to show)
  uint32_t getDisplayLeftMs() {
    portENTER_CRITICAL(&timerMux);
    uint32_t leftMs = displayPhaseLeftMs;
    portEXIT_CRITICAL(&timerMux);
    return leftMs;
  }

private:
  // GREEN -> GREEN_BLINK -> YELLOW -> RED -> RED_YELLOW -> GREEN
  enum State {
    GREEN,
    GREEN_BLINK,
    YELLOW,
    RED,
    RED_YELLOW,
    BLINK_YELLOW
  };

  // Bit masks for direct register writes (valid for GPIO 0..31)
  static constexpr uint32_t LED_RED_MASK    = 1UL << LED_RED_PIN;
  static constexpr uint32_t LED_YELLOW_MASK = 1UL << LED_YELLOW_PIN;
  static constexpr uint32_t LED_GREEN_MASK  = 1UL << LED_GREEN_PIN;
  static constexpr uint32_t LEDS_ALL_MASK   = LED_RED_MASK | LED_YELLOW_MASK | LED_GREEN_MASK;

  // Total green time shown on the display: steady green + blinking
  static constexpr uint32_t GREEN_TOTAL_MS =
    DURATION_GREEN_MS + 2 * GREEN_BLINK_COUNT * BLINK_HALF_PERIOD_MS;

  // Shared with the ISR -> volatile, protected by timerMux
  portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
  volatile uint32_t timeLeftMs = 0;          // ms until the end of the phase (or of one blink step)
  volatile uint32_t displayPhaseLeftMs = 0;  // ms until the color changes (0 = display off)
  volatile bool isPhaseEnded = false;        // set by tick(), handled in update()

  // Used only from loop() -> not volatile
  State currentState = GREEN;
  int blinkStepsLeft = 0;
  bool blinkLedOn = false;

  void setStepTimer(uint32_t ms) {
    portENTER_CRITICAL(&timerMux);
    timeLeftMs = ms;
    portEXIT_CRITICAL(&timerMux);
  }

  void setDisplayTimer(uint32_t ms) {
    portENTER_CRITICAL(&timerMux);
    displayPhaseLeftMs = ms;
    portEXIT_CRITICAL(&timerMux);
  }

  // W1TS sets the masked pins HIGH, W1TC sets them LOW; other pins are not touched.
  void setLights(bool red, bool yellow, bool green) {
    uint32_t onMask = (red ? LED_RED_MASK : 0) | (yellow ? LED_YELLOW_MASK : 0) | (green  ? LED_GREEN_MASK : 0);
    REG_WRITE(GPIO_OUT_W1TS_REG, onMask);
    REG_WRITE(GPIO_OUT_W1TC_REG, LEDS_ALL_MASK & ~onMask);
  }

  // One blink step: toggle the LED and wait for the next half-period.
  void toggleBlink(uint32_t pinMask) {
    blinkLedOn = !blinkLedOn;
    REG_WRITE(blinkLedOn ? GPIO_OUT_W1TS_REG : GPIO_OUT_W1TC_REG, pinMask);
    setStepTimer(BLINK_HALF_PERIOD_MS);
  }

  static const char *getStateName(State state) {
    switch (state) {
      case GREEN:        return "GREEN";
      case GREEN_BLINK:  return "GREEN BLINK";
      case YELLOW:       return "YELLOW";
      case RED:          return "RED";
      case RED_YELLOW:   return "RED+YELLOW";
      case BLINK_YELLOW: return "BLINK YELLOW";
    }
    return "?";
  }

  // Enter a new state: turn on the LEDs and set the phase duration.
  void startPhase(State state) {
    currentState = state;
    Serial.printf("[%lu s] %s\n", millis() / 1000, getStateName(state));

    switch (state) {
      case GREEN:
        setLights(false, false, true);
        setDisplayTimer(GREEN_TOTAL_MS);
        setStepTimer(DURATION_GREEN_MS);
        break;
      case GREEN_BLINK:
        // Green toggles every half-period: N blinks = 2*N steps.
        // The display timer is not reset: the countdown continues from GREEN.
        blinkLedOn = true;
        blinkStepsLeft = 2 * GREEN_BLINK_COUNT;
        setLights(false, false, true);
        setStepTimer(BLINK_HALF_PERIOD_MS);
        break;
      case YELLOW:
        setLights(false, true, false);
        setDisplayTimer(DURATION_YELLOW_MS);
        setStepTimer(DURATION_YELLOW_MS);
        break;
      case RED:
        setLights(true, false, false);
        setDisplayTimer(DURATION_RED_MS);
        setStepTimer(DURATION_RED_MS);
        break;
      case RED_YELLOW:
        setLights(true, true, false);
        setDisplayTimer(0);  // display is off during red+yellow
        setStepTimer(DURATION_RED_YELLOW_MS);
        break;
      case BLINK_YELLOW:
        blinkLedOn = true;
        setLights(false, true, false);
        setDisplayTimer(0);  // no countdown at an unregulated intersection
        setStepTimer(BLINK_HALF_PERIOD_MS);
        break;
    }
  }

  // Decides what comes next when a phase (or one blink step) has ended.
  void handlePhaseEnd() {
    switch (currentState) {
      case GREEN:
        startPhase(GREEN_BLINK);
        break;
      case GREEN_BLINK:
        blinkStepsLeft--;
        if (blinkStepsLeft > 0) {
          toggleBlink(LED_GREEN_MASK);
        } else {
          startPhase(YELLOW);
        }
        break;
      case YELLOW:
        startPhase(RED);
        break;
      case RED:
        startPhase(RED_YELLOW);
        break;
      case RED_YELLOW:
        startPhase(GREEN);
        break;
      case BLINK_YELLOW:
        toggleBlink(LED_YELLOW_MASK);
        break;
    }
  }
};


TrafficLight trafficLight;
CountdownDisplay countdownDisplay;
hw_timer_t *lightTimer = NULL;

// timerAttachInterrupt needs a plain function, so this one forwards to the object.
void IRAM_ATTR onTimer() {
  trafficLight.tick();
}

void setup() {
  Serial.begin(115200);

  countdownDisplay.begin();
  trafficLight.begin(UNREGULATED_MODE);

  lightTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(lightTimer, &onTimer, true);
  timerAlarmWrite(lightTimer, 1000, true);
  timerAlarmEnable(lightTimer);
}

void loop() {
  trafficLight.update();

  uint32_t leftMs = trafficLight.getDisplayLeftMs();
  countdownDisplay.showDigit(leftMs > 0 ? (leftMs + 999) / 1000 : -1);
}