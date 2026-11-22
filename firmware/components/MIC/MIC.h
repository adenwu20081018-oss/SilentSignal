#ifndef MIC_H
#define MIC_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"

/* INMP441：WS=IO15，SCK=IO16，SD=IO17。无 MCLK。L/R 接地为左声道，接 3V3 为右声道。 */
#define MIC_GPIO_WS       GPIO_NUM_15
#define MIC_GPIO_SCK      GPIO_NUM_16
#define MIC_GPIO_SD       GPIO_NUM_17
#define MIC_SAMPLE_RATE   16000

esp_err_t MIC_Init(void);

/* 读一块立体声数据，左右峰值各为 0~32767（取 32bit 槽的高 16 位） */
esp_err_t MIC_ReadPeak(uint16_t *left, uint16_t *right);

/* 一块约 16ms 的帧，统计较大的那一路。tone/low 为 0~100，crest_x10 = 峰值/平均 × 10 */
typedef struct {
    uint16_t peak;
    uint16_t avg;
    uint16_t f0_hz;
    uint16_t crest_x10;
    uint8_t tone_x100;
    uint8_t low_x100;
    uint16_t samples;
} mic_frame_t;

esp_err_t MIC_ReadFrame(mic_frame_t *out);

#endif
