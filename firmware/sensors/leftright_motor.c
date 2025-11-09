#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"

#include "leftright_motor.h"

static const char *TAG = "servo";

// Convert servo angle to pulse width for MG996R
// Accepts angles from -90 to +90 degrees
static uint32_t servo_per_degree_init(int32_t degree_of_rotation)
{
    // Clamp to valid range
    if (degree_of_rotation < SERVO_MIN_DEGREE) degree_of_rotation = SERVO_MIN_DEGREE;
    if (degree_of_rotation > SERVO_MAX_DEGREE) degree_of_rotation = SERVO_MAX_DEGREE;
    
    // Map angle (-90 to +90) to pulse width (1000 to 2000 µs)
    // Formula: pulse = center + (angle * range / total_degrees)
    uint32_t cal_pulsewidth = SERVO_CENTER_PULSEWIDTH + 
                              ((degree_of_rotation * (SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH)) / 
                               (SERVO_MAX_DEGREE - SERVO_MIN_DEGREE));
    
    return cal_pulsewidth;
}

// Initialize servo
void servo_init(void)
{
    // 1. Configure MCPWM
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_PIN);
    
    // 2. Initial MCPWM configuration
    mcpwm_config_t pwm_config = {
        .frequency = 50,    // Servo frequency = 50Hz (20ms period)
        .cmpr_a = 0,        // Duty cycle of PWMxA = 0
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
    
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    
    ESP_LOGI(TAG, "Servo initialized on GPIO %d", SERVO_PIN);
}

// Write angle to servo (accepts -90 to +90 degrees for MG996R)
void servo_write(int32_t angle)
{
    uint32_t pulsewidth = servo_per_degree_init(angle);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, pulsewidth);
    
    ESP_LOGI(TAG, "Servo angle: %ld degrees (pulse: %lu us)", angle, pulsewidth);
}

//void app_main(void)
void leftright_motor_task(void *pvParameters) 
{
    (void) pvParameters;
    // Initialize servo (like myservo.attach(9))
    servo_init();
    
    // Configure UART (Serial.begin(9600))
    uart_config_t uart_config = {
        .baud_rate = 9600,  // Same as Arduino Serial.begin(9600)
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    
    ESP_LOGI(TAG, "UART initialized at 9600 baud");
    ESP_LOGI(TAG, "Waiting for commands...");
    ESP_LOGI(TAG, "  0xFF -> 30 degrees");
    ESP_LOGI(TAG, "  0x77 -> -30 degrees");
    ESP_LOGI(TAG, "  0x69 -> 0 degrees (center)");
    
    uint8_t incomingByte;
    
    // Main loop (like Arduino loop())
    while (1) {
        // Check if data is available (like Serial.available())
        int len = uart_read_bytes(UART_NUM, &incomingByte, 1, 20 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            ESP_LOGI(TAG, "Received byte: 0x%02X", incomingByte);
            
            // Process incoming byte
            if (incomingByte == 0xFF) {
                servo_write(30);   // 30° right of center
            }
            else if (incomingByte == 0x77) {
                servo_write(-30);  // 30° left of center
            }
            else if (incomingByte == 0x69) {
                servo_write(0);    // Center position
            }
            else if (incomingByte == 0x20) {
                servo_write(-5);    // Slow left
            }
            else if (incomingByte == 0x30) {
                servo_write(5);    // Slow right
            }
            else {
                ESP_LOGW(TAG, "Unknown command: 0x%02X", incomingByte);
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay
    }
}