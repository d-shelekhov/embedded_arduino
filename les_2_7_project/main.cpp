#include <Arduino.h>

constexpr int LED_RED_PIN    = 17;
constexpr int LED_YELLOW_PIN = 15;
constexpr int LED_GREEN_PIN  = 16;

constexpr uint32_t DURATION_GREEN_MS      = 4000;
constexpr uint32_t DURATION_YELLOW_MS     = 2000;
constexpr uint32_t DURATION_RED_MS        = 5000;
constexpr uint32_t DURATION_RED_YELLOW_MS = 1500;
constexpr uint32_t BLINK_HALF_PERIOD_MS   = 300;
constexpr int      GREEN_BLINK_COUNT      = 3;

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
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;  // защита данных, общих с прерыванием

volatile State currentState = GREEN;
volatile uint32_t timeLeftMs = 1;  // How many milliseconds remain until the end of the phase (or flashing cycle)
volatile int blinkStepsLeft = 0;
volatile bool blinkLedOn = false;
volatile bool hasStateChanged = false;

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
      break;
    case RED:
      setLights(true, false, false);
      timeLeftMs = DURATION_RED_MS;
      break;
    case RED_YELLOW:
      setLights(true, true, false);
      timeLeftMs = DURATION_RED_YELLOW_MS;
      break;
    case BLINK_YELLOW:
      blinkLedOn = true;
      setLights(false, true, false);
      timeLeftMs = BLINK_HALF_PERIOD_MS;
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
  if (timeLeftMs > 0 && --timeLeftMs == 0) {
    handlePhaseEnd();
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

  lightTimer = timerBegin(0, 80, true); // 80 MHz / 80 = 1 MHz
  timerAttachInterrupt(lightTimer, &countDownPhaseTime, true);

  startPhase(GREEN);

  timerAlarmWrite(lightTimer, 1000, true);
  timerAlarmEnable(lightTimer);
}

void loop() {
  bool stateChanged;
  State state;
  portENTER_CRITICAL(&timerMux);
  stateChanged = hasStateChanged;
  hasStateChanged = false;
  state = currentState;
  portEXIT_CRITICAL(&timerMux);

  if (stateChanged) {
    Serial.printf("[%lu s] %s\n", millis() / 1000, getStateName(state));
  }
}