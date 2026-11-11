#include "buzzer.h"
#include <Arduino.h>
#include "config.h"

#define LEDC_BASE_FREQ_HZ   2000
#define LEDC_RESOLUTION_BITS 10

typedef struct {
    uint16_t frequency_hz;
    uint16_t duration_ms;
} melody_note_t;

static const melody_note_t MELODY_UNLOCKED[] = {{523, 120}, {659, 120}, {784, 120}, {1047, 300}, {0, 0}};
static const melody_note_t MELODY_WRONG[]    = {{220, 250}, {0, 60}, {180, 400}, {0, 0}};
static const melody_note_t MELODY_ALARM[]    = {{1200, 200}, {800, 200}, {1200, 200}, {800, 200}, {1200, 200}, {800, 200}, {1200, 200}, {800, 200}, {0, 0}};
static const melody_note_t MELODY_LOCKED[]   = {{784, 100}, {523, 200}, {0, 0}};

static const melody_note_t *const MELODIES[] = {
    /* BUZZER_MELODY_UNLOCKED */ MELODY_UNLOCKED,
    /* BUZZER_MELODY_WRONG    */ MELODY_WRONG,
    /* BUZZER_MELODY_ALARM    */ MELODY_ALARM,
    /* BUZZER_MELODY_LOCKED   */ MELODY_LOCKED,
};

void buzzer_init(void)
{
    ledcSetup(BUZZER_LEDC_CHANNEL, LEDC_BASE_FREQ_HZ, LEDC_RESOLUTION_BITS);
    ledcAttachPin(BUZZER_GPIO, BUZZER_LEDC_CHANNEL);
    ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);
}

void buzzer_play_melody(buzzer_melody_t melody)
{
    for (const melody_note_t *note = MELODIES[melody]; note->duration_ms; note++) {
        ledcWriteTone(BUZZER_LEDC_CHANNEL, note->frequency_hz);   /* 0 = silence (pause) */
        delay(note->duration_ms);
    }
    ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);
}