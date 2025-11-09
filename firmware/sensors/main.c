#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/ledc.h"
#include "esp_task_wdt.h"

#include "init_led.h"
#include "btn_led.h"
#include "servo_rotate.h"
#include "flywheel.h"

// Define the GPIO pins
#define LED_GPIO GPIO_NUM_15 
#define BUTTON_GPIO GPIO_NUM_4 

#define INIT_LED_GPIO GPIO_NUM_2

#define IN2 GPIO_NUM_19
#define IN1 GPIO_NUM_21
#define ENA GPIO_NUM_18     


TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t ledTaskHandle2 = NULL;
TaskHandle_t ledTaskHandle3 = NULL;
TaskHandle_t ledTaskHandle4 = NULL;

const char *TAG = "APP";

// Queue for button interrupts
QueueHandle_t btn_evt_queue = NULL;


// ISR push GPIO num into queue
void IRAM_ATTR button_isr(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(btn_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}


void app_main(void) {

    esp_task_wdt_deinit(); //deint task watchdog

    //GPIO Config
    //configure LED outputs
    gpio_config_t led_io_conf = {
    .intr_type = GPIO_INTR_DISABLE, //disable interrupts for led
    .mode = GPIO_MODE_OUTPUT, //set pin output
    .pin_bit_mask = (1ULL << LED_GPIO), //set specific LED pin
    .pull_up_en = 0,
    .pull_down_en = 0,
    };
    gpio_config(&led_io_conf); //apply config 
    gpio_set_level(LED_GPIO, 0); //dont float

    //config button input
    gpio_config_t btn_io_conf = {
    .intr_type = GPIO_INTR_NEGEDGE, // fire on press (high->low)
    .mode = GPIO_MODE_INPUT, //set pin output
    .pin_bit_mask = (1ULL << BUTTON_GPIO), //set specific btn pin
    .pull_down_en = 0, //disablle pulldown 
    .pull_up_en = 1, //en pup
    };
    gpio_config(&btn_io_conf); //apply config 

    gpio_config_t init_led_io_conf = {
        .pin_bit_mask = 1ULL << INIT_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&init_led_io_conf);
    gpio_set_level(INIT_LED_GPIO, 0);

    
    // ISR service and handler 
    gpio_install_isr_service(0); // installs the shared ISR service
    gpio_isr_handler_add(BUTTON_GPIO, button_isr, (void*) BUTTON_GPIO);

    // Queue for button events 
    btn_evt_queue = xQueueCreate(8, sizeof(uint32_t));


//RTOS tasks

xTaskCreate(btnLED_task, "btnLED_task", 2 * configMINIMAL_STACK_SIZE, NULL, 3, &ledTaskHandle);

xTaskCreate(init_led_task, "init_led_task", 2 * configMINIMAL_STACK_SIZE, NULL, 2, &ledTaskHandle2);

xTaskCreate(flywheel_task, "flywheel_task", 2048, NULL, 4, &ledTaskHandle3);

xTaskCreate(&servoRotate_task, "servoRotate_task", 2048, NULL, 5, &ledTaskHandle4);


}
