#ifndef BTN_LED_H
#define BTN_LED_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define LED_GPIO GPIO_NUM_15 // GPIO2 for LED
#define BUTTON_GPIO GPIO_NUM_4 // 4 for button


void btnLED_task(void *pvParameters);

extern QueueHandle_t btn_evt_queue;

#endif
