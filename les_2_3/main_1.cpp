#include <Arduino.h>

const uint8_t LED1_PIN = 8;
const uint8_t LED2_PIN = 9;
const uint8_t LED3_PIN = 10;

const unsigned long LED1_INTERVAL_MS = 200;
const unsigned long LED2_INTERVAL_MS = 500;
const unsigned long LED3_INTERVAL_MS = 1000;

struct BlinkTask {
  uint8_t pin;
  unsigned long interval;
  unsigned long lastToggle;
  bool state;
};

BlinkTask tasks[] = {
  { LED1_PIN, LED1_INTERVAL_MS, 0, LOW },
  { LED2_PIN, LED2_INTERVAL_MS, 0, LOW },
  { LED3_PIN, LED3_INTERVAL_MS, 0, LOW },
};
const uint8_t TASK_COUNT = sizeof(tasks) / sizeof(tasks[0]);

void runBlinkTask(BlinkTask &task, unsigned long now) {
  if (now - task.lastToggle >= task.interval) {
    task.lastToggle += task.interval;
    task.state = !task.state;
    digitalWrite(task.pin, task.state);
  }
}

void setup() {
  for (uint8_t i = 0; i < TASK_COUNT; i++) {
    pinMode(tasks[i].pin, OUTPUT);
    digitalWrite(tasks[i].pin, LOW);
  }
}

void loop() {
  unsigned long now = millis();

  for (uint8_t i = 0; i < TASK_COUNT; i++) {
    runBlinkTask(tasks[i], now);
  }
}