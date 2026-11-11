#include "indicator.h"
#include <Arduino.h>
#include "config.h"

typedef enum {
    RED_PATTERN_STEADY_OFF,
    RED_PATTERN_STEADY_ON,
    RED_PATTERN_COUNTED_BLINKS,
    RED_PATTERN_ENDLESS_BLINK,
} red_pattern_t;

static red_pattern_t s_red_pattern = RED_PATTERN_STEADY_ON;
static bool          s_red_pin_is_high = true;     /* що зараз на піні червоного */
static uint8_t       s_blinks_remaining = 0;       /* спалахів ще треба показати (лише COUNTED_BLINKS) */
static uint32_t      s_next_toggle_ms = 0;         /* коли перемкнути червоний (лише при миганні) */

static void set_red(bool lit)
{
    s_red_pin_is_high = lit;
    digitalWrite(LED_RED_GPIO, lit ? HIGH : LOW);
}

static void set_green(bool lit)
{
    digitalWrite(LED_GREEN_GPIO, lit ? HIGH : LOW);
}

void indicator_init(void)
{
    pinMode(LED_RED_GPIO, OUTPUT);
    pinMode(LED_GREEN_GPIO, OUTPUT);
    indicator_show_locked();
}

void indicator_show_locked(void)
{
    s_red_pattern = RED_PATTERN_STEADY_ON;
    set_red(true);
    set_green(false);
}

void indicator_show_unlocked(void)
{
    s_red_pattern = RED_PATTERN_STEADY_OFF;
    set_red(false);
    set_green(true);
}

void indicator_show_blocked(void)
{
    s_red_pattern = RED_PATTERN_ENDLESS_BLINK;
    set_green(false);
    set_red(true);
    s_next_toggle_ms = millis() + LED_BLOCKED_BLINK_MS;
}

void indicator_blink_red(uint8_t count)
{
    if (count == 0) return;
    if (s_red_pattern != RED_PATTERN_STEADY_ON && s_red_pattern != RED_PATTERN_COUNTED_BLINKS) return;

    s_red_pattern = RED_PATTERN_COUNTED_BLINKS;
    s_blinks_remaining = count;
    set_red(false);
    s_next_toggle_ms = millis() + LED_BLINK_OFF_MS;
}

void indicator_update(void)
{
    if (s_red_pattern != RED_PATTERN_COUNTED_BLINKS && s_red_pattern != RED_PATTERN_ENDLESS_BLINK) return;

    if ((int32_t)(millis() - s_next_toggle_ms) < 0) return;

    if (s_red_pattern == RED_PATTERN_ENDLESS_BLINK) {
        set_red(!s_red_pin_is_high);
        s_next_toggle_ms += LED_BLOCKED_BLINK_MS;
        return;
    }

    if (!s_red_pin_is_high) {                           /* була пауза -> спалах */
        set_red(true);
        s_blinks_remaining--;
        if (s_blinks_remaining == 0) {
            s_red_pattern = RED_PATTERN_STEADY_ON;      /* серія закінчилась, далі світиться */
        } else {
            s_next_toggle_ms += LED_BLINK_ON_MS;
        }
    } else {                                            /* світився -> нова пауза */
        set_red(false);
        s_next_toggle_ms += LED_BLINK_OFF_MS;
    }
}