#include <Arduino.h>

constexpr int LED_RED_PIN    = 17;
constexpr int LED_YELLOW_PIN = 15;
constexpr int LED_GREEN_PIN  = 16;

constexpr int DISPLAY_SEGMENT_PINS[7] = {4, 5, 6, 7, 8, 9, 10};
constexpr int DISPLAY_SEGMENT_ON = LOW;

constexpr uint8_t DISPLAY_DIGIT_MASKS[10] = {
  0b0111111,
  0b0000110,
  0b1011011,
  0b1001111, 
  0b1100110,
  0b1101101, 
  0b1111101,
  0b0000111,
  0b1111111,
  0b1101111
};

constexpr uint32_t DURATION_GREEN_MS      = 4000;
constexpr uint32_t DURATION_YELLOW_MS     = 2000;
constexpr uint32_t DURATION_RED_MS        = 5000;
constexpr uint32_t DURATION_RED_YELLOW_MS = 1500;
constexpr uint32_t BLINK_HALF_PERIOD_MS   = 300;
constexpr int      GREEN_BLINK_COUNT      = 3;

constexpr uint32_t GREEN_TOTAL_MS = DURATION_GREEN_MS + 2 * GREEN_BLINK_COUNT * BLINK_HALF_PERIOD_MS;

// GREEN -> GREEN_BLINK -> YELLOW -> RED -> RED_YELLOW -> GREEN
enum State {
  GREEN,
  GREEN_BLINK,
  YELLOW,
  RED,
  RED_YELLOW,
  BLINK_YELLOW
};

hw_timer_t *lightTimer = NULL;
// Spinlock: makes reads of ISR-shared data in loop() atomic (interrupts off inside the critical section)
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile State currentState = GREEN;
volatile uint32_t timeLeftMs = 1;  // How many milliseconds remain until the end of the phase (or flashing cycle)
volatile int blinkStepsLeft = 0;
volatile bool blinkLedOn = false;
volatile bool hasStateChanged = false;
volatile uint32_t displayPhaseLeftMs = 0;

void IRAM_ATTR setLights(bool red, bool yellow, bool green) {
  digitalWrite(LED_RED_PIN, red);
  digitalWrite(LED_YELLOW_PIN, yellow);
  digitalWrite(LED_GREEN_PIN, green);
}

// One flashing cycle: toggle the LED and wait for the next half-period.
void IRAM_ATTR toggleBlink(int pin) {
  blinkLedOn = !blinkLedOn;
  digitalWrite(pin, blinkLedOn);
  timeLeftMs = BLINK_HALF_PERIOD_MS;
}

// Transition to a new state: turn on the LEDs and set the phase duration.
void IRAM_ATTR startPhase(State state) {
  currentState = state;
  hasStateChanged = true;

  switch (state) {
    case GREEN:
      setLights(false, false, true);
      timeLeftMs = DURATION_GREEN_MS;
      displayPhaseLeftMs = GREEN_TOTAL_MS;
      break;
    case GREEN_BLINK:
      // The green light toggles every half-period: N flashes = 2*N steps.
      blinkLedOn = true;
      blinkStepsLeft = 2 * GREEN_BLINK_COUNT;
      setLights(false, false, true);
      timeLeftMs = BLINK_HALF_PERIOD_MS;
      break;
    case YELLOW:
      setLights(false, true, false);
      timeLeftMs = DURATION_YELLOW_MS;
      displayPhaseLeftMs = DURATION_YELLOW_MS;
      break;
    case RED:
      setLights(true, false, false);
      timeLeftMs = DURATION_RED_MS;
      displayPhaseLeftMs = DURATION_RED_MS;
      break;
    case RED_YELLOW:
      setLights(true, true, false);
      timeLeftMs = DURATION_RED_YELLOW_MS;
      displayPhaseLeftMs = 0; 
      break;
    case BLINK_YELLOW:
      blinkLedOn = true;
      setLights(false, true, false);
      timeLeftMs = BLINK_HALF_PERIOD_MS;
      displayPhaseLeftMs = 0;
      break;
  }
}

// Called when a phase (or a single blinking cycle) has ended: determines what to do next.
void IRAM_ATTR handlePhaseEnd() {
  switch (currentState) {
    case GREEN:
      startPhase(GREEN_BLINK);
      break;
    case GREEN_BLINK:
      blinkStepsLeft--;
      if (blinkStepsLeft > 0) {
        toggleBlink(LED_GREEN_PIN);
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
      toggleBlink(LED_YELLOW_PIN);
      break;
  }
}

// Timer interrupt: fires every 1 ms, counts down the time until the end of the phase.
void IRAM_ATTR countDownPhaseTime() {
  if (displayPhaseLeftMs > 0) 
    displayPhaseLeftMs--;
  if (timeLeftMs > 0 && --timeLeftMs == 0) {
    handlePhaseEnd();
  }
}

void showDisplayDigit(int digit) {
  uint8_t mask = (digit >= 0 && digit <= 9) ? DISPLAY_DIGIT_MASKS[digit] : 0;
  for (int i = 0; i < 7; i++) {
    bool isSegmentEnabled = (mask >> i) & 1;
    digitalWrite(DISPLAY_SEGMENT_PINS[i], isSegmentEnabled ? DISPLAY_SEGMENT_ON : !DISPLAY_SEGMENT_ON);
  }
}

const char *getStateName(State state) {
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

void setup() {
  Serial.begin(115200);

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);

  for (int pin : DISPLAY_SEGMENT_PINS) pinMode(pin, OUTPUT);
  showDisplayDigit(-1); // Turn off traffic light digit monitor first

  lightTimer = timerBegin(0, 80, true); // 80 MHz / 80 = 1 MHz
  timerAttachInterrupt(lightTimer, &countDownPhaseTime, true);

  startPhase(GREEN);

  timerAlarmWrite(lightTimer, 1000, true);
  timerAlarmEnable(lightTimer);
}

void loop() {
  bool stateChanged;
  State state;
  uint32_t leftMs; 

  portENTER_CRITICAL(&timerMux);
  stateChanged = hasStateChanged;
  hasStateChanged = false;
  state = currentState;
  leftMs = displayPhaseLeftMs;
  portEXIT_CRITICAL(&timerMux);

  static int shownDisplayDigit = -1;
  int digit = leftMs > 0 ? (leftMs + 999) / 1000 : -1;
  if (digit != shownDisplayDigit) {   // rewrite only of digit changed
    shownDisplayDigit = digit;
    showDisplayDigit(digit);
  }

  if (stateChanged) {
    Serial.printf("[%lu s] %s\n", millis() / 1000, getStateName(state));
  }
}