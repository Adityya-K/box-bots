#ifndef SERVO_ROTATE_H
#define SERVO_ROTATE_H

#define IN2 GPIO_NUM_19
#define IN1 GPIO_NUM_21
#define ENA GPIO_NUM_18 // Must REMOVE ENA jumper on L298N if using this pin
#define BUTTON_GPIO GPIO_NUM_4 // 4 for button

void servoRotate_task(void *pvParameters);

#endif
