#pragma once

#define SECRET_CODE                 "4271"
#define CODE_LENGTH                 4  
#define MAX_ATTEMPTS                3

#define ENCODER_GPIO_CLK            4
#define ENCODER_GPIO_DT             5
#define ENCODER_GPIO_BUTTON         6

#define BUZZER_GPIO                 7
#define BUZZER_LEDC_CHANNEL         0

#define RELAY_GPIO                  15
#define RELAY_ACTIVE_LEVEL          0
#define RELAY_UNLOCK_PULSE_MS       2000

#define ENCODER_INVERT_DIRECTION    0
#define ENCODER_STEPS_PER_DETENT    4
#define ENCODER_BUTTON_DEBOUNCE_US  20000

#define LED_RED_GPIO                10
#define LED_GREEN_GPIO              9

#define LED_BLINK_OFF_MS            200
#define LED_BLINK_ON_MS             200
#define LED_BLOCKED_BLINK_MS        100