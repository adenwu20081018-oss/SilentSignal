
#ifndef __LED_H_
#define __LED_H_

#include <driver/gpio.h>


esp_err_t  LED_Init_OUT(gpio_num_t num);

void GPIO_Toggle(gpio_num_t gpio_num);

#endif