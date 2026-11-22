#include "PWM.h"

#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "PWM";

esp_err_t PWM_Init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config %s", esp_err_to_name(err));
        return err;
    }

    ledc_channel_config_t ch = {
        .gpio_num = PWM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    err = ledc_channel_config(&ch);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "PWM %u Hz on IO%d", (unsigned)PWM_FREQ_HZ, (int)PWM_GPIO);
    return ESP_OK;
}

esp_err_t PWM_SetDuty(uint32_t duty)
{
    if (duty > PWM_DUTY_MAX) {
        duty = PWM_DUTY_MAX;
    }
    esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    if (err != ESP_OK) {
        return err;
    }
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
