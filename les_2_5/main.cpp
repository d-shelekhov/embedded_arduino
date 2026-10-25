#include <Arduino.h>
#include "esp_task_wdt.h"

#define DEBUG_MODE 1 // 1 = DEBUG MODE, 0 = WORK MODE
#define WATCHDOG_TIMEOUT_SEC 5


#if DEBUG_MODE
  #define FAN_PERIOD_SEC 20
  #define FAN_RUN_DURATION_SEC 5
#else
  #define FAN_PERIOD_SEC 3600
  #define FAN_RUN_DURATION_SEC 900
#endif

#define FAN_PIN 4
#define STATUS_LED_PIN 5

// The timer counts in microseconds, so convert seconds to microseconds.
// (uint64_t) prevents overflow: one hour = 3,600,000,000 us
#define MICROS_PER_SECOND 1000000ULL
#define FAN_RUN_DURATION_MICROS ((uint64_t)FAN_RUN_DURATION_SEC * MICROS_PER_SECOND)
#define FAN_IDLE_DURATION_MICROS ((uint64_t)(FAN_PERIOD_SEC - FAN_RUN_DURATION_SEC) * MICROS_PER_SECOND)

hw_timer_t *fanTimer = NULL;
volatile bool isFanRunning = false;
volatile bool hasStateChanged = false;

void IRAM_ATTR restartTimer(uint64_t intervalMicros) {
  timerWrite(fanTimer, 0);
  timerAlarmWrite(fanTimer, intervalMicros, false);
  timerAlarmEnable(fanTimer);
}

void IRAM_ATTR setFanState(bool is_fan_running) {
  isFanRunning = is_fan_running;
  digitalWrite(FAN_PIN, is_fan_running);
  digitalWrite(STATUS_LED_PIN, is_fan_running);
  hasStateChanged = true;
}

void IRAM_ATTR switchFanPhase() {
  if (isFanRunning) {
    // The run phase just ended: stop the fan and wait until the period is over.
    setFanState(false);
    restartTimer(FAN_IDLE_DURATION_MICROS); // idle until the end of the period
  } else {
    // The idle phase just ended: start the fan and let it run for the set time.
    setFanState(true);
    restartTimer(FAN_RUN_DURATION_MICROS); // run for the set duration
  }
}

void setup() {
  Serial.begin(115200);

  esp_task_wdt_init(WATCHDOG_TIMEOUT_SEC, true);
  esp_task_wdt_add(NULL);   // subscribe the current task (loop) to the watchdog

  pinMode(FAN_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);

  fanTimer = timerBegin(0, 80, true); // 80 MHz / 80 = 1 MHz
  timerAttachInterrupt(fanTimer, &switchFanPhase, true);

  setFanState(true);
  restartTimer(FAN_RUN_DURATION_MICROS);
}

void loop() {
  esp_task_wdt_reset();

  if (hasStateChanged) {
    hasStateChanged = false;
    Serial.printf("[%lu s] FAN: %s\n", millis() / 1000, isFanRunning ? "ON" : "OFF");
  }
}
