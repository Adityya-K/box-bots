#ifndef SERVO_H
#define SERVO_H

#include "driver/mcpwm.h"
#include "driver/uart.h"


// MG996R Servo specifications
#define SERVO_MIN_PULSEWIDTH       1000  // 1ms = -90°
#define SERVO_MAX_PULSEWIDTH       2000  // 2ms = +90°
#define SERVO_CENTER_PULSEWIDTH    1500  // 1.5ms = 0°
#define SERVO_MIN_DEGREE           -90
#define SERVO_MAX_DEGREE           90

#define SERVO_PIN 13
#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024

// Function prototypes
void servo_init(void);
void servo_write(int32_t angle);
void leftright_motor_task(void *pvParameters);

#endif // SERVO_H