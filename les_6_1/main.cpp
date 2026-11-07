#include <cstdio>
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

struct LedParams {
    gpio_num_t  pin;
    uint32_t    interval_ms;
    const char *name;
};

static constexpr LedParams LEDS[] = {
    { GPIO_NUM_8, 200,  "LED1" },
    { GPIO_NUM_9, 500,  "LED2" },
    { GPIO_NUM_10, 1000, "LED3" },
};

static void blink_task(void *pvParameters)
{
    // Cast void* to LedParams*, dereference it, and copy this task's settings into a local variable
    LedParams led = *static_cast<const LedParams *>(pvParameters);

    gpio_reset_pin(led.pin);
    gpio_set_direction(led.pin, GPIO_MODE_OUTPUT);

    bool level = false;
    gpio_set_level(led.pin, level);

    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(led.interval_ms);

    while (true) {
        vTaskDelayUntil(&last_wake, period);
        level = !level;
        gpio_set_level(led.pin, level);
    }
}

extern "C" void app_main(void)
{
    std::printf("Starting FreeRTOS tasks...\n");
    for (const auto &led : LEDS) {
        xTaskCreate(blink_task, led.name, 2048, const_cast<LedParams *>(&led), 1, nullptr);
    }
}