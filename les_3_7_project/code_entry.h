#pragma once
/*
* code_entry - logic for entering a code by rotating the encoder.
*
* Input rules:
*  - Rotating in one direction increments the current digit from 0 to 9 (after 9, it resets to 0);
*  - Changing direction confirms the digit; the first click in the new direction
*    only confirms and sets the new digit to 0 (it does not increase it);
*  - After the last digit, changing direction completes the code entry.
*/
#include <stdbool.h>
#include <stdint.h>

#define CODE_MAX_LENGTH 8

/* Code input state. */
typedef struct {
    uint8_t confirmed_digits[CODE_MAX_LENGTH];
    uint8_t code_length;
    uint8_t digit_index;
    uint8_t current_digit;
    int8_t  direction;
} code_entry_t;

typedef enum {
    CODE_ENTRY_DIGIT_CHANGED,
    CODE_ENTRY_NEXT_DIGIT,
    CODE_ENTRY_COMPLETE,
} code_entry_event_t;

/* Sets the code length and resets the input. Call once at the start. */
void code_entry_init(code_entry_t *entry, uint8_t code_length);

/* Resets the input (numbers, position, direction). Does not change the length of the code. */
void code_entry_reset(code_entry_t *entry);

/* Processes a single encoder click.
* direction: +1 (clockwise) or -1 (counterclockwise). */
code_entry_event_t code_entry_rotate(code_entry_t *entry, int direction);

/* Compares the entered code with the secret (a string of digits). Call after
 * CODE_ENTRY_COMPLETE. Execution time does not depend on where the discrepancy occurs. */
bool code_entry_matches(const code_entry_t *entry, const char *secret_code);