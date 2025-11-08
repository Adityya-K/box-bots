
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "dht.h" //added component (library)

#include "sdkconfig.h"
#include "driver/ledc.h"

// Define the GPIO pins for the LED and the button
#define LED_GPIO GPIO_NUM_15 // GPIO2 for LED
#define BUTTON_GPIO GPIO_NUM_4 // 4 for button

#define INIT_LED_GPIO GPIO_NUM_2

TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t ledTaskHandle2 = NULL;

static const char *TAG = "APP";

/* Queue for button interrupts */
static QueueHandle_t btn_evt_queue = NULL;


/* --- ISR: push GPIO num into queue --- */
static void IRAM_ATTR button_isr(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(btn_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

#define IN1 GPIO_NUM_19
#define IN2 GPIO_NUM_21
#define ENA GPIO_NUM_18     // Must REMOVE ENA jumper on L298N if using this pin

void flywheel_task(void *pvParameters) {
    // Configure as outputs
    gpio_config_t io = {
        .pin_bit_mask = (1ULL<<IN1) | (1ULL<<IN2) | (1ULL<<ENA),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);

    // Enable motor driver
    gpio_set_level(ENA, 1);   // motor enable HIGH

    // Set direction: forward
    gpio_set_level(IN1, 1);
    gpio_set_level(IN2, 0);

    // Motor runs forever
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void btnLED_task(void *pvParameters) {
    (void) pvParameters;
    const TickType_t debounce_ms = 30; // simple debounce
    const TickType_t debounce_ticks = pdMS_TO_TICKS(debounce_ms);


    //state variables
    int led_state = 0; //0 = off
    gpio_set_level(LED_GPIO, led_state);

    uint32_t io_num;
    
    //main loop
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

/* Task: simple heartbeat blink on INIT_LED_GPIO */
static void init_led_task(void *pvParameters) {
    (void) pvParameters;
    int led_state = 0;

    gpio_set_level(INIT_LED_GPIO, led_state);

    while (1) {
        led_state = !led_state;
        gpio_set_level(INIT_LED_GPIO, led_state);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


static void servoRotate_task(void *pvParameters) {
    (void)pvParameters;

    int max_duty = (1 << 15) - 1; //(15 your choice) (1 << 15 = 2^15)

    const float min_s = 0.0005f, max_s = 0.0025f, total_s = 0.02f; //180 rotation

    int  duty = (int)(max_duty * (min_s / total_s)); // vary from 1639 to 327
    int step = 14; //each iteration, the duty increments itself by this step (your choice)
    int total_cycles = (((max_duty * (max_s / total_s)) - (max_duty * (min_s / total_s))) / (step)); //total 117 iterations
    bool pos_direction = true; //true up, false down
    int iteration_time = 10; //msec (your choice) how fast sweep hapens

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
        .timer_sel = LEDC_TIMER_0, //same as timer config
    };
    ledc_channel_config(&ledc_conf);


    int i;
    while (1) {
        for (i = 0; i < total_cycles; i++) {
            //if direction is pos, duty inc itself, otherwise reduce value
            pos_direction ? (duty += step) : (duty -= step);
            //set new duty and update in each iteration
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

            vTaskDelay(iteration_time/portTICK_PERIOD_MS);
        }
        //change dir
        
        pos_direction = !pos_direction;
    }

}



void app_main(void) {

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

    
    // --- ISR service and handler ---
    gpio_install_isr_service(0); // installs the shared ISR service
    gpio_isr_handler_add(BUTTON_GPIO, button_isr, (void*) BUTTON_GPIO);

    // --- Queue for button events ---
    btn_evt_queue = xQueueCreate(8, sizeof(uint32_t));





xTaskCreate(btnLED_task, "btnLED_task", 2 * configMINIMAL_STACK_SIZE, NULL, 3, &ledTaskHandle);

xTaskCreate(init_led_task, "init_led_task", 2 * configMINIMAL_STACK_SIZE, NULL, 2, &ledTaskHandle2);

xTaskCreate(flywheel_task, "flywheel_task", 2048, NULL, 4, NULL);


xTaskCreate(&servoRotate_task, "servoRotate_task", 2048, NULL, 5, NULL);


}
