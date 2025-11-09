#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "sdkconfig.h"

#include "flywheel.h"


void flywheel_task(void *pvParameters) {

    gpio_config_t ena_io = {
        .pin_bit_mask = (1ULL<<ENA),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0, .pull_down_en = 0, .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&ena_io);
    gpio_set_level(ENA, 1);// enable channel A

    gpio_config_t in2_io = {
        .pin_bit_mask = (1ULL<<IN2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0, .pull_down_en = 0, .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&in2_io);
    gpio_set_level(IN2, 0);// fixed direction

    // LEDC 20 kHz on IN1 
    ledc_timer_config_t tmr = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_1,
        .duty_resolution = LEDC_TIMER_10_BIT, 
        .freq_hz         = 20000,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&tmr);

    ledc_channel_config_t ch = {
        .gpio_num   = IN1,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .timer_sel  = LEDC_TIMER_1,
        .duty       = 0,// start stopped
        .hpoint     = 0,
        .intr_type  = LEDC_INTR_DISABLE
    };
    ledc_channel_config(&ch);

    // --- State ---
    const int DUTY_MAX   = (1 << 10) - 1;// 1023
    const int STEP       = 16;// ramp granularity
    const TickType_t DT  = pdMS_TO_TICKS(10);
    int current_duty     = 0;
    int target_duty      = 0;// start stopped
    bool motor_on        = false;

    while (1) {
        // BUTTON active-low press to toggle
        if (gpio_get_level(BUTTON_GPIO) == 0) {
            vTaskDelay(pdMS_TO_TICKS(30));// debounce
            if (gpio_get_level(BUTTON_GPIO) == 0) {
                // wait for release so one press = one toggle
                while (gpio_get_level(BUTTON_GPIO) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(5));
                }
                motor_on   = !motor_on;
                target_duty = motor_on ? DUTY_MAX : 0;
            }
        }

        // RAMP current_duty -> target_duty (non-blocking) 
        if (current_duty < target_duty) {
            current_duty += STEP;
            if (current_duty > target_duty) current_duty = target_duty;
        } else if (current_duty > target_duty) {
            current_duty -= STEP;
            if (current_duty < target_duty) current_duty = target_duty;
        }

        // apply PWM
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, current_duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

        vTaskDelay(DT);
    }
}
