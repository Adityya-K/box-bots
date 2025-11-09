#include "init_led.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void init_led_task(void *pvParameters)
{
    (void) pvParameters;

    int led_state = 0;
    gpio_set_level(INIT_LED_GPIO, led_state);

    while (1) {
        led_state = !led_state;
        gpio_set_level(INIT_LED_GPIO, led_state);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}