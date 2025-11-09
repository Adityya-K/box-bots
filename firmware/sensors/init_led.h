#ifndef INIT_LED_H
#define INIT_LED_H

#include "driver/gpio.h"

#define INIT_LED_GPIO GPIO_NUM_2

void init_led_task(void *pvParameters);


#endif