#include "door_lock.h"
#include <Arduino.h>
#include "config.h"

#define RELAY_LEVEL_ON   (RELAY_ACTIVE_LEVEL ? HIGH : LOW)
#define RELAY_LEVEL_OFF  (RELAY_ACTIVE_LEVEL ? LOW : HIGH)

static bool s_is_locked = true;

void door_lock_init(void)
{
    digitalWrite(RELAY_GPIO, RELAY_LEVEL_OFF);
    pinMode(RELAY_GPIO, OUTPUT);
    s_is_locked = true;
}

void door_lock_unlock(void)
{
    if (!s_is_locked) return;

    digitalWrite(RELAY_GPIO, RELAY_LEVEL_ON);
    delay(RELAY_UNLOCK_PULSE_MS);
    digitalWrite(RELAY_GPIO, RELAY_LEVEL_OFF);
    s_is_locked = false;
}

void door_lock_lock(void)
{
    digitalWrite(RELAY_GPIO, RELAY_LEVEL_OFF);
    s_is_locked = true;
}