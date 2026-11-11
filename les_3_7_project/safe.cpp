#include "safe.h"
#include <Arduino.h>
#include "code_entry.h"
#include "config.h"
#include "buzzer.h"
#include "door_lock.h"
#include "indicator.h"

static_assert(CODE_LENGTH >= 1 && CODE_LENGTH <= CODE_MAX_LENGTH, "CODE_LENGTH must be 1..8");
static_assert(sizeof(SECRET_CODE) - 1 == CODE_LENGTH, "SECRET_CODE length must equal CODE_LENGTH");
static_assert(MAX_ATTEMPTS >= 1, "MAX_ATTEMPTS must be >= 1");


typedef enum {
    SAFE_STATE_ENTERING_CODE,
    SAFE_STATE_UNLOCKED,
    SAFE_STATE_BLOCKED,
} safe_state_t;

static code_entry_t s_code_entry;                           /* current input status */
static safe_state_t s_state = SAFE_STATE_ENTERING_CODE;
static int          s_failed_attempts = 0;

/* ---------- Console ----------
* The input sequence looks like “CODE: 4 2 7 1”: the current digit is overwritten
* with the \b (backspace) character; the confirmed digit remains in place. */
/* Starts a new input and prints the initial string “CODE: 0”. */

static void console_start_new_entry(void)
{
    code_entry_reset(&s_code_entry);
    Serial.print("CODE: 0");
}

static void unlock_safe(void)
{
    s_state = SAFE_STATE_UNLOCKED;
    indicator_show_unlocked();
    door_lock_unlock();
    buzzer_play_melody(BUZZER_MELODY_UNLOCKED);
    Serial.println("Code CORRECT. OPEN. Button: Close.");
}

static void block_safe(void)
{
    s_state = SAFE_STATE_BLOCKED;
    indicator_show_blocked();
    for (int i = 0; i < 3; i++) {
        buzzer_play_melody(BUZZER_MELODY_ALARM);
    }
    Serial.println("Attempts left. SYSTEM LOCKED until reboot.");
}

/* Counts an unsuccessful attempt (incorrect code or resetting with the button).
 * Either blocks the attempt or gives another chance and restarts the input. */
static void register_failed_attempt(const char *reason)
{
    s_failed_attempts++;
    Serial.print(reason);
    Serial.print(". Attempt");
    Serial.print(s_failed_attempts);
    Serial.print("/");
    Serial.println(MAX_ATTEMPTS);

    if (s_failed_attempts >= MAX_ATTEMPTS) {
        block_safe();
        return;
    }

    buzzer_play_melody(BUZZER_MELODY_WRONG);
    console_start_new_entry();
}

/* ---------- Event handling based on state ---------- */

/* “Code entry” state: button = reset (this is an attempt), rotation = digits. */
static void handle_event_entering_code(encoder_event_t event)
{
    if (event == ENCODER_EVENT_BUTTON_PRESSED) {
        Serial.println();                       /* Close the “CODE: ...” line */
        register_failed_attempt("RESET ... ");
        return;
    }

    int direction = (event == ENCODER_EVENT_CLOCKWISE) ? +1 : -1;
    switch (code_entry_rotate(&s_code_entry, direction)) {
    case CODE_ENTRY_DIGIT_CHANGED:
        Serial.print('\b');                       /* Erase the old number ... */
        Serial.print(s_code_entry.current_digit); /* ... and write a new one in its place */
        break;
    case CODE_ENTRY_NEXT_DIGIT:
        Serial.print(" 0");                       /* new number on the right */
        /* The confirmed digit—the last one in confirmed_digits—is the number of times the red light flashes */
        indicator_blink_red(s_code_entry.confirmed_digits[s_code_entry.digit_index - 1]);
        break;
    case CODE_ENTRY_COMPLETE:
        Serial.println();
        if (code_entry_matches(&s_code_entry, SECRET_CODE)) {
            unlock_safe();
        } else {
            register_failed_attempt("WRONG CODE");
        }
        break;
    }
}


static void handle_event_unlocked(encoder_event_t event)
{
    if (event != ENCODER_EVENT_BUTTON_PRESSED) return;

    s_failed_attempts = 0;
    s_state = SAFE_STATE_ENTERING_CODE;
    door_lock_lock();
    indicator_show_locked();
    Serial.println("LOCKED.");
    buzzer_play_melody(BUZZER_MELODY_LOCKED);
    console_start_new_entry();
}

void safe_init(void)
{
    code_entry_init(&s_code_entry, CODE_LENGTH);
    console_start_new_entry();
}

void safe_handle_event(encoder_event_t event)
{
    switch (s_state) {
    case SAFE_STATE_ENTERING_CODE: handle_event_entering_code(event); break;
    case SAFE_STATE_UNLOCKED:      handle_event_unlocked(event);      break;
    case SAFE_STATE_BLOCKED:       break;
    }
}