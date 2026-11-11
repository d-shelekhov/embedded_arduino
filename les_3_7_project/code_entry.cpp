#include "code_entry.h"
#include <string.h>

void code_entry_reset(code_entry_t *entry)
{
    memset(entry->confirmed_digits, 0, sizeof(entry->confirmed_digits));
    entry->digit_index = 0;
    entry->current_digit = 0;
    entry->direction = 0;
}

void code_entry_init(code_entry_t *entry, uint8_t code_length)
{
    entry->code_length = code_length;
    code_entry_reset(entry);
}

code_entry_event_t code_entry_rotate(code_entry_t *entry, int direction)
{
    /* First click (direction == 0) or same direction -> increase the number. */
    bool same_direction = (entry->direction == 0 || entry->direction == direction);
    if (same_direction) {
        entry->direction = (int8_t)direction;
        entry->current_digit = (entry->current_digit + 1) % 10;   /* 9 -> 0 по колу */
        return CODE_ENTRY_DIGIT_CHANGED;
    }

    /* The trend has changed: we are confirming the current digit. */
    entry->confirmed_digits[entry->digit_index++] = entry->current_digit;
    if (entry->digit_index >= entry->code_length) {
        return CODE_ENTRY_COMPLETE;     /* це була остання цифра */
    }

    /* We start the next number at 0; this click alone does not increase it. */
    entry->direction = (int8_t)direction;
    entry->current_digit = 0;
    return CODE_ENTRY_NEXT_DIGIT;
}

bool code_entry_matches(const code_entry_t *entry, const char *secret_code)
{
    /* We accumulate the difference across all digits and only then look at the result.
     * An early exit at the first discrepancy would reveal, based on the time taken, how many digits were guessed correctly. */
    uint8_t difference = 0;
    for (uint8_t i = 0; i < entry->code_length; i++) {
        difference |= entry->confirmed_digits[i] ^ (uint8_t)(secret_code[i] - '0');
    }
    return difference == 0;
}