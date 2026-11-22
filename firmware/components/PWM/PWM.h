#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"

/* 一路 LEDC PWM，接到 IO7。10 位分辨率，占空比 0~1024（1024 = 100%）。 */
#define PWM_GPIO          GPIO_NUM_7
#define PWM_FREQ_HZ       1000
#define PWM_DUTY_MAX      1024

esp_err_t PWM_Init(void);
esp_err_t PWM_SetDuty(uint32_t duty);

#endif
