#ifndef FLYWHEEL_H
#define FLYWHEEL_H

#include "driver/gpio.h"

// Re-declare the same pin macros used in your task
#define IN2 GPIO_NUM_19
#define IN1 GPIO_NUM_21
#define ENA GPIO_NUM_18     
#define BUTTON_GPIO GPIO_NUM_4

void flywheel_task(void *pvParameters);

#endif
