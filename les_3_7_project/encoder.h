#pragma once
#include <stdbool.h>

typedef enum {
    ENCODER_EVENT_CLOCKWISE,        /* one click clockwise */
    ENCODER_EVENT_COUNTERCLOCKWISE, /* one click counterclockwise */
    ENCODER_EVENT_BUTTON_PRESSED,
} encoder_event_t;

void encoder_init(void);

bool encoder_poll(encoder_event_t *event);