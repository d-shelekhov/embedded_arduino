#pragma once
#include <stdint.h>

void indicator_init(void);

void indicator_show_locked(void);

void indicator_show_unlocked(void);

void indicator_show_blocked(void);

void indicator_blink_red(uint8_t count);

void indicator_update(void);