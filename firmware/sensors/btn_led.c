#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "btn_led.h"

extern const char *TAG;
extern QueueHandle_t btn_evt_queue;

void btnLED_task(void *pvParameters) {
    (void) pvParameters;
    const TickType_t debounce_ms = 30; // simple debounce
    const TickType_t debounce_ticks = pdMS_TO_TICKS(debounce_ms);

    // state variables
    int led_state = 0; // 0 = off
    gpio_set_level(LED_GPIO, led_state);

    uint32_t io_num;

    // main loop (logic unchanged)
    while (1) {
        if (xQueueReceive(btn_evt_queue, &io_num, portMAX_DELAY)) {
            // basic debounce: wait a bit, then confirm button is still pressed (low)
            vTaskDelay(debounce_ticks);

            int level = gpio_get_level(BUTTON_GPIO);

            if (level == 0) { // pressed (active-low)
                led_state = !led_state;
                gpio_set_level(LED_GPIO, led_state);
                ESP_LOGI(TAG, "Button press -> LED %s", led_state ? "ON" : "OFF");

                // wait until released to avoid repeat toggles while held
                while (gpio_get_level(BUTTON_GPIO) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(5));
                }
            }
        }
    }
}
