#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "sdkconfig.h"

#include "servo_rotate.h"


void servoRotate_task(void *pvParameters) {
    (void)pvParameters;

    int max_duty = (1 << 15) - 1;// 15-bit resolution (0..32767)

    const float min_s = 0.001f;
    const float max_s = 0.002f;
    const float total_s = 0.02f;// 20 ms frame for servo

    int duty = (int)(max_duty * (min_s / total_s)); // start at 1 ms
    int step = 14;                                   // sweep increment
    int total_cycles =
        ((max_duty * (max_s / total_s)) -
         (max_duty * (min_s / total_s))) / step;

    bool pos_direction = true; // true = sweep up, false = sweep down
    int iteration_time = 10; // ms per step

    // LEDC config (50 Hz servo PWM)
    ledc_timer_config_t timer_conf = {
        .duty_resolution = LEDC_TIMER_15_BIT,
        .freq_hz = 50,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ledc_conf = {
        .channel = LEDC_CHANNEL_0,
        .duty = duty,
        .gpio_num = 25,
        .intr_type = LEDC_INTR_DISABLE,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0,
    };
    ledc_channel_config(&ledc_conf);

    int i;
    while (1) {

        //  Sweep once 
        for (i = 0; i < total_cycles; i++) {

            if (pos_direction) {
                duty -= step;   
            } else {
                duty += step;   
            }

            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

            vTaskDelay(iteration_time / portTICK_PERIOD_MS);
        }

        // HOLD 
        // do nothing — servo stays at new position
        // WAIT for button press 
        while (gpio_get_level(BUTTON_GPIO) != 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
        }

        // debounce
        vTaskDelay(pdMS_TO_TICKS(30));

        // wait for release
        while (gpio_get_level(BUTTON_GPIO) == 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
        }

        // wait 2 seconds (flywheel spin-up)
        vTaskDelay(pdMS_TO_TICKS(2000));

        // Reverse direction 
        pos_direction = !pos_direction;
    }
}
