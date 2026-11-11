#include <Arduino.h>
#include "encoder.h"
#include "config.h"

/* ---------- Circular Event Queue ----------
 * One writes (interrupt), one reads (loop) — so no locks are needed.
 * The interrupt modifies only s_head, and the loop modifies only s_tail. The queue is empty when
 * head == tail; it is full when head + 1 == tail (one slot is always free).
 * If the queue is full, a new event is discarded (this does not happen in practice:
 * loop() removes events faster than they can be generated manually). */
#define EVENT_QUEUE_SIZE 16

static volatile encoder_event_t s_events[EVENT_QUEUE_SIZE];
static volatile uint8_t s_head = 0;
static volatile uint8_t s_tail = 0;

static volatile uint8_t s_previous_phase_state;
static volatile int     s_accumulated_steps;

static void IRAM_ATTR push_event(encoder_event_t event)
{
    uint8_t next = (s_head + 1) % EVENT_QUEUE_SIZE;
    if (next == s_tail) return;         /* queue is full */
    s_events[s_head] = event;           /* First, we record the event... */
    s_head = next;                      /* ... then we publish it */
}

bool encoder_poll(encoder_event_t *event)
{
    if (s_tail == s_head) return false;
    *event = s_events[s_tail];
    s_tail = (s_tail + 1) % EVENT_QUEUE_SIZE;
    return true;
}

/* ---------- Rotation Decoder ---------- */

/* Decoder state. Only the phase interrupt handler is triggered. */


/* Quadrature decoder table. Index = (previous state << 2) | current,
 * state = (CLK << 1) | DT. +1 / -1 - step in that direction, 0 - no movement or missed
 * state (contact bounce). Because of this, phase bounce does not require a separate filter. */
static const int8_t QUADRATURE_DELTA[16] = {0, -1, 1, 0, 1, 0, 0, -1,
                                            -1, 0, 0, 1, 0, 1, -1, 0};

static inline uint8_t IRAM_ATTR read_phase_state(void)
{
    return (digitalRead(ENCODER_GPIO_CLK) << 1) | digitalRead(ENCODER_GPIO_DT);
}

static void IRAM_ATTR phase_isr(void)
{
    uint8_t state = read_phase_state();
    if (state == s_previous_phase_state) return;    /* debounce */

    s_accumulated_steps += QUADRATURE_DELTA[(s_previous_phase_state << 2) | state];
    s_previous_phase_state = state;

    if (s_accumulated_steps >= ENCODER_STEPS_PER_DETENT ||
        s_accumulated_steps <= -ENCODER_STEPS_PER_DETENT) {
        /* A full click has been registered. */
        int direction = s_accumulated_steps > 0 ? 1 : -1;
        s_accumulated_steps = 0;
        if (ENCODER_INVERT_DIRECTION) direction = -direction;
        push_event(direction > 0 ? ENCODER_EVENT_CLOCKWISE
                                 : ENCODER_EVENT_COUNTERCLOCKWISE);
    } else if (state == 3) {
        s_accumulated_steps = 0;
    }
}

static volatile uint32_t s_last_button_edge_us;

static void IRAM_ATTR button_isr(void)
{
    uint32_t now_us = micros();
    bool quiet = (uint32_t)(now_us - s_last_button_edge_us) > ENCODER_BUTTON_DEBOUNCE_US;
    s_last_button_edge_us = now_us;     /* Update on EVERY edge: the rattle prolongs the “silence” */

    /* We consider a drop (LOW = pressed) following a period of silence to be an event. */
    if (quiet && digitalRead(ENCODER_GPIO_BUTTON) == LOW) {
        push_event(ENCODER_EVENT_BUTTON_PRESSED);
    }
}

void encoder_init(void)
{
    /* Inputs pulled up to VCC: at rest, they read HIGH; the pin is connected to GND. */
    pinMode(ENCODER_GPIO_CLK, INPUT_PULLUP);
    pinMode(ENCODER_GPIO_DT, INPUT_PULLUP);
    pinMode(ENCODER_GPIO_BUTTON, INPUT_PULLUP);

    s_previous_phase_state = read_phase_state();
    s_accumulated_steps = 0;
    s_last_button_edge_us = micros();

    /* CHANGE = interruption both on the falling edge and on the rising edge. */
    attachInterrupt(digitalPinToInterrupt(ENCODER_GPIO_CLK), phase_isr, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_GPIO_DT), phase_isr, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_GPIO_BUTTON), button_isr, CHANGE);
}