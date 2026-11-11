#pragma once

typedef enum {
    BUZZER_MELODY_UNLOCKED,
    BUZZER_MELODY_WRONG,
    BUZZER_MELODY_ALARM,
    BUZZER_MELODY_LOCKED,
} buzzer_melody_t;

void buzzer_init(void);

void buzzer_play_melody(buzzer_melody_t melody);