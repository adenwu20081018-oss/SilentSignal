#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include "LED.h"
#include "OLED.h"
#include "MIC.h"
#include "PWM.h"

/* 一帧 16ms。门铃看约 0.25 秒内音高是否稳住，敲门看包络是否一下冲高再落下去。 */
#define WARMUP_FRAMES     12
#define BELL_WIN          16
#define MOTOR_DUTY        256
#define OTHER_HOLD_FRAMES 125
#define BAR_FULL_SCALE    8192

#define MODE_NORMAL       0
#define MODE_KNOCK        1
#define MODE_BELL         2

#define SND_NONE          0
#define SND_KNOCK         1
#define SND_BELL          2
#define SND_OTHER         3

#define RGB_OFF           0
#define RGB_RED           1
#define RGB_GREEN         2
#define RGB_BLUE          3

#define KEY_CONFIRM       GPIO_NUM_5
#define KEY_MODE          GPIO_NUM_6

typedef struct {
    uint16_t level;
    uint8_t mode;
    uint8_t sound;
    uint8_t alert;
    uint8_t rgb;
    uint8_t mic_ok;
    uint8_t ack;
} view_t;

typedef struct {
    gpio_num_t pin;
    uint8_t stable;
    uint8_t raw_ticks;
    uint16_t hold;
} key_t;

static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static view_t s_view = { .mic_ok = 1 };

static const char *const s_mode_text[] = {
    "MODE NORMAL",
    "MODE KNOCK",
    "MODE DOORBELL",
};

static const char *const s_sound_text[] = {
    "SOUND NONE",
    "SOUND KNOCK",
    "SOUND DOORBELL",
    "SOUND OTHER",
};

static const gpio_num_t s_rgb[3] = {
    GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_11
};

static void view_store_level(uint16_t level, uint8_t ok)
{
    portENTER_CRITICAL(&s_lock);
    s_view.level = level;
    s_view.mic_ok = ok;
    portEXIT_CRITICAL(&s_lock);
}

static uint8_t view_take_ack(void)
{
    uint8_t ack;
    portENTER_CRITICAL(&s_lock);
    ack = s_view.ack;
    s_view.ack = 0;
    portEXIT_CRITICAL(&s_lock);
    return ack;
}

static uint8_t view_publish(uint8_t raw, uint8_t block_motor)
{
    uint8_t shown = raw;

    portENTER_CRITICAL(&s_lock);
    if (s_view.mode == MODE_KNOCK && raw == SND_BELL) {
        shown = SND_OTHER;
    }
    if (s_view.mode == MODE_BELL && raw == SND_KNOCK) {
        shown = SND_OTHER;
    }
    s_view.sound = shown;
    if ((shown == SND_KNOCK || shown == SND_BELL) && !block_motor) {
        s_view.alert = 1;
        s_view.rgb = (shown == SND_KNOCK) ? RGB_RED : RGB_GREEN;
    } else if (shown == SND_OTHER && !s_view.alert) {
        s_view.rgb = RGB_BLUE;
    }
    portEXIT_CRITICAL(&s_lock);
    return shown;
}

static view_t view_load(void)
{
    view_t out;
    portENTER_CRITICAL(&s_lock);
    out = s_view;
    portEXIT_CRITICAL(&s_lock);
    return out;
}

static void view_confirm(void)
{
    portENTER_CRITICAL(&s_lock);
    s_view.alert = 0;
    s_view.rgb = RGB_OFF;
    s_view.ack = 1;
    portEXIT_CRITICAL(&s_lock);
}

static void view_next_mode(void)
{
    portENTER_CRITICAL(&s_lock);
    s_view.mode = (uint8_t)((s_view.mode + 1) % 3);
    portEXIT_CRITICAL(&s_lock);
}

static void view_other_done(void)
{
    portENTER_CRITICAL(&s_lock);
    if (!s_view.alert && s_view.rgb == RGB_BLUE) {
        s_view.rgb = RGB_OFF;
    }
    portEXIT_CRITICAL(&s_lock);
}

static void rgb_init(void)
{
    unsigned i;
    uint64_t mask = 0;
    gpio_config_t cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    for (i = 0; i < 3; i++) {
        mask |= (1ULL << s_rgb[i]);
        gpio_reset_pin(s_rgb[i]);
    }
    cfg.pin_bit_mask = mask;
    gpio_config(&cfg);
    for (i = 0; i < 3; i++) {
        gpio_set_drive_capability(s_rgb[i], GPIO_DRIVE_CAP_3);
        gpio_sleep_sel_dis(s_rgb[i]);
        gpio_set_level(s_rgb[i], 0);
    }
}

static void rgb_apply(uint8_t which)
{
    gpio_set_level(GPIO_NUM_13, which == RGB_RED);
    gpio_set_level(GPIO_NUM_12, which == RGB_GREEN);
    gpio_set_level(GPIO_NUM_11, which == RGB_BLUE);
}

static void key_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << KEY_CONFIRM) | (1ULL << KEY_MODE),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_reset_pin(KEY_CONFIRM);
    gpio_reset_pin(KEY_MODE);
    gpio_config(&cfg);
}

/* 返回 1 表示一次短按松开。按键按下时为低电平。 */
static int key_short(key_t *key)
{
    int raw = gpio_get_level(key->pin) == 0;
    int released = 0;

    if (raw == key->stable) {
        key->raw_ticks = 0;
    } else if (++key->raw_ticks >= 3) {
        key->stable = (uint8_t)raw;
        key->raw_ticks = 0;
        if (key->stable) {
            key->hold = 0;
        } else {
            released = 1;
        }
    }
    if (key->stable && key->hold < 1000) {
        key->hold++;
    }
    return released && key->hold >= 3 && key->hold < 80;
}

static uint16_t gate_of(uint16_t noise, unsigned mul, uint16_t floor)
{
    uint32_t gate = (uint32_t)noise * mul;
    if (gate < floor) {
        gate = floor;
    }
    if (gate > 20000u) {
        gate = 20000u;
    }
    return (uint16_t)gate;
}

static int frame_is_tone(const mic_frame_t *frame, uint16_t noise)
{
    return frame->peak >= gate_of(noise, 3, 450) &&
           frame->tone_x100 >= 70 &&
           frame->crest_x10 > 0 &&
           frame->crest_x10 <= 40 &&
           frame->f0_hz >= 350 &&
           frame->f0_hz <= 2800;
}

static void mic_task(void *arg)
{
    mic_frame_t hist[BELL_WIN];
    uint8_t hist_n = 0;
    uint8_t block = 0;
    uint8_t warmup = WARMUP_FRAMES;
    uint8_t bell_on = 0;
    uint8_t burst = 0;
    uint8_t quiet = 0;
    uint8_t tagged = 0;
    uint8_t knock_cool = 0;
    uint8_t noise_seen = 0;
    uint16_t noise = 4000;
    uint16_t other_left = 0;
    uint16_t burst_peak = 0;

    (void)arg;
    memset(hist, 0, sizeof(hist));
    while (1) {
        mic_frame_t frame;
        esp_err_t err = MIC_ReadFrame(&frame);
        int hot;
        int tonal = 0;
        uint16_t fmin = 65535;
        uint16_t fmax = 0;
        unsigned i;
        uint8_t shown;
        const mic_frame_t *hit;

        if (view_take_ack()) {
            block = (burst > 0 || bell_on) ? 1 : 0;
        }
        if (err != ESP_OK) {
            view_store_level(0, 0);
            continue;
        }
        view_store_level(frame.peak, 1);
        if (other_left) {
            other_left--;
            if (other_left == 0) {
                view_other_done();
            }
        }

        if (warmup) {
            if (frame.avg > 0 && frame.avg < noise) {
                noise = frame.avg;
                noise_seen = 1;
            }
            warmup--;
            if (warmup == 0 && !noise_seen) {
                noise = 300;
            }
            continue;
        }
        if (frame.avg > 0 && frame.avg < noise * 2u) {
            noise = (uint16_t)((noise * 15u + frame.avg) / 16u);
        }
        if (noise < 30) {
            noise = 30;
        }

        memmove(&hist[1], &hist[0], sizeof(hist[0]) * (BELL_WIN - 1));
        hist[0] = frame;
        if (hist_n < BELL_WIN) {
            hist_n++;
        }

        if (hist_n >= BELL_WIN) {
            for (i = 0; i < BELL_WIN; i++) {
                if (!frame_is_tone(&hist[i], noise)) {
                    continue;
                }
                tonal++;
                if (hist[i].f0_hz < fmin) {
                    fmin = hist[i].f0_hz;
                }
                if (hist[i].f0_hz > fmax) {
                    fmax = hist[i].f0_hz;
                }
            }
            if (tonal >= 8 && (uint16_t)(fmax - fmin) <= 500) {
                if (!bell_on) {
                    shown = view_publish(SND_BELL, block);
                    tagged = 1;
                    other_left = (shown == SND_OTHER) ? OTHER_HOLD_FRAMES : 0;
                }
                bell_on = 1;
            } else if (tonal < 3) {
                bell_on = 0;
            }
        }

        if (knock_cool) {
            knock_cool--;
        }
        hit = &hist[5];
        if (!bell_on && knock_cool == 0 && hist_n >= 7 &&
            hit->peak >= gate_of(noise, 4, 700) &&
            hit->avg >= gate_of(noise, 3, 120) &&
            hist[6].avg > 0 && hit->avg >= hist[6].avg * 2u &&
            hit->avg >= hist[4].avg &&
            hist[1].avg * 2u < hit->avg &&
            hit->tone_x100 < 82 &&
            hit->crest_x10 >= 22 &&
            hit->f0_hz >= 40 &&
            hit->f0_hz <= 2200) {
            shown = view_publish(SND_KNOCK, block);
            tagged = 1;
            knock_cool = 10;
            other_left = (shown == SND_OTHER) ? OTHER_HOLD_FRAMES : 0;
        }

        hot = frame.peak >= gate_of(noise, 4, 600);
        if (hot) {
            if (burst < 250) {
                burst++;
            }
            quiet = 0;
            if (frame.peak > burst_peak) {
                burst_peak = frame.peak;
            }
        } else if (burst) {
            quiet++;
            if (quiet >= 5) {
                if (!tagged && !bell_on && burst >= 8 && burst_peak >= gate_of(noise, 6, 1200)) {
                    shown = view_publish(SND_OTHER, block);
                    other_left = (shown == SND_OTHER) ? OTHER_HOLD_FRAMES : 0;
                }
                burst = 0;
                quiet = 0;
                tagged = 0;
                burst_peak = 0;
                block = 0;
            }
        }
    }
}

static void ui_task(void *arg)
{
    char prev_mode[20] = "";
    char prev_sound[20] = "";
    char prev_alert[20] = "";
    uint8_t prev_bar = 0xFF;

    (void)arg;
    while (1) {
        view_t now = view_load();
        const char *mode = s_mode_text[now.mode % 3];
        const char *sound = now.mic_ok ? s_sound_text[now.sound % 4] : "MIC READ FAIL";
        const char *alert = now.alert ? "ALERT ON" : "ALERT OFF";
        uint16_t span = (uint16_t)((uint32_t)now.level * 128u / BAR_FULL_SCALE);
        uint8_t bar;

        if (span > 128) {
            span = 128;
        }
        bar = (uint8_t)(span & ~3u);

        if (strcmp(mode, prev_mode) != 0) {
            OLED_ShowLine16(0, mode);
            snprintf(prev_mode, sizeof(prev_mode), "%s", mode);
        }
        if (strcmp(sound, prev_sound) != 0) {
            OLED_ShowLine16(2, sound);
            snprintf(prev_sound, sizeof(prev_sound), "%s", sound);
        }
        if (strcmp(alert, prev_alert) != 0) {
            OLED_ShowLine16(4, alert);
            snprintf(prev_alert, sizeof(prev_alert), "%s", alert);
        }
        if (bar != prev_bar) {
            OLED_ShowLevel(bar);
            prev_bar = bar;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void ctrl_task(void *arg)
{
    key_t confirm = { .pin = KEY_CONFIRM };
    key_t mode = { .pin = KEY_MODE };
    uint8_t motor = 0xFF;
    uint8_t rgb = 0xFF;
    uint8_t led = 0;
    uint8_t tick = 0;

    (void)arg;
    while (1) {
        view_t now;

        if (key_short(&confirm)) {
            view_confirm();
        }
        if (key_short(&mode)) {
            view_next_mode();
        }

        now = view_load();
        if (now.alert != motor) {
            motor = now.alert;
            PWM_SetDuty(motor ? MOTOR_DUTY : 0);
        }
        if (now.rgb != rgb) {
            rgb = now.rgb;
            rgb_apply(rgb);
        }

        if (++tick >= 50) {
            tick = 0;
            led = !led;
            gpio_set_level(GPIO_NUM_10, led);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    LED_Init_OUT(GPIO_NUM_10);
    gpio_set_level(GPIO_NUM_10, 1);
    rgb_init();
    key_init();

    OLED_Init();
    OLED_Clear();
    OLED_ShowLine16(0, "MODE NORMAL");
    OLED_ShowLine16(2, "SOUND NONE");
    OLED_ShowLine16(4, "ALERT OFF");
    OLED_ShowLevel(0);

    if (PWM_Init() != ESP_OK) {
        OLED_ShowLine16(4, "PWM INIT FAIL");
    }

    xTaskCreatePinnedToCore(ui_task, "ui", 4096, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(ctrl_task, "ctrl", 3072, NULL, 6, NULL, 0);

    if (MIC_Init() != ESP_OK) {
        OLED_ShowLine16(2, "MIC INIT FAIL");
        view_store_level(0, 0);
        return;
    }
    xTaskCreatePinnedToCore(mic_task, "mic", 4096, NULL, 5, NULL, 1);
}
