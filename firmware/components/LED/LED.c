#include <stdio.h>
#include "LED.h"

esp_err_t LED_Init_OUT(gpio_num_t num)
{
    esp_err_t err;
    gpio_config_t ESP_gpio_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pin_bit_mask = 1ull << num,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };

    err = gpio_config(&ESP_gpio_config);
    return err;
}

void GPIO_Toggle(gpio_num_t gpio_num)
{

    int  num = gpio_get_level(gpio_num);
    printf("gpio_get_level %d\r\n",num);
    gpio_set_level(gpio_num, 1 - num);
}

