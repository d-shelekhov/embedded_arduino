/*
 * КРОК 8 (Arduino): автомат сейфа зі спробами, мелодіями, реле і світлодіодами.
 *
 * setup() ініціалізує модулі, loop() забирає події енкодера і передає їх
 * автомату (safe). Мигання світлодіодів іде без delay(), через indicator_update().
 */
#include <Arduino.h>
#include "buzzer.h"
#include "door_lock.h"
#include "encoder.h"
#include "indicator.h"
#include "safe.h"

void setup()
{
    Serial.begin(115200);
    uint32_t start_ms = millis();
    while (!Serial && millis() - start_ms < 3000) {
        delay(10);
    }

    Serial.println();
    Serial.print("START OK...");

    door_lock_init();
    indicator_init();
    buzzer_init();
    encoder_init();
    safe_init();
}

void loop()
{
    indicator_update();

    encoder_event_t event;
    while (encoder_poll(&event)) {
        safe_handle_event(event);
    }
}