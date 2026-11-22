#include "MIC.h"

#include <math.h>
#include "driver/i2s_std.h"
#include "esp_log.h"

#define MIC_FFT_N 256

static float s_re[MIC_FFT_N];
static float s_im[MIC_FFT_N];
static float s_win[MIC_FFT_N];
static uint8_t s_win_ok;

static void mic_window_init(void)
{
    int i;

    if (s_win_ok) {
        return;
    }
    for (i = 0; i < MIC_FFT_N; i++) {
        s_win[i] = 0.5f * (1.0f - cosf(6.2831853f * (float)i / (float)(MIC_FFT_N - 1)));
    }
    s_win_ok = 1;
}

static void fft_256(float *re, float *im)
{
    int i;
    int j = 0;

    for (i = 1; i < MIC_FFT_N; i++) {
        int bit = MIC_FFT_N >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            float tr = re[i];
            float ti = im[i];
            re[i] = re[j];
            im[i] = im[j];
            re[j] = tr;
            im[j] = ti;
        }
    }

    for (int len = 2; len <= MIC_FFT_N; len <<= 1) {
        float ang = -6.2831853f / (float)len;
        float wlen_re = cosf(ang);
        float wlen_im = sinf(ang);
        for (i = 0; i < MIC_FFT_N; i += len) {
            float wr = 1.0f;
            float wi = 0.0f;
            for (int k = 0; k < len / 2; k++) {
                int u = i + k;
                int v = u + len / 2;
                float tr = wr * re[v] - wi * im[v];
                float ti = wr * im[v] + wi * re[v];
                re[v] = re[u] - tr;
                im[v] = im[u] - ti;
                re[u] += tr;
                im[u] += ti;
                tr = wr * wlen_re - wi * wlen_im;
                wi = wr * wlen_im + wi * wlen_re;
                wr = tr;
            }
        }
    }
}

/* 256 点频谱：基频、纯音集中度、500Hz 以下能量占比 */
static void mic_spectrum(const int16_t *x, mic_frame_t *out)
{
    int i;
    float mean = 0.0f;
    float tot = 0.0f;
    float low = 0.0f;
    float emax = 0.0f;
    float band;
    int kmax = 1;

    mic_window_init();
    for (i = 0; i < MIC_FFT_N; i++) {
        mean += (float)x[i];
    }
    mean /= (float)MIC_FFT_N;
    for (i = 0; i < MIC_FFT_N; i++) {
        s_re[i] = ((float)x[i] - mean) * s_win[i];
        s_im[i] = 0.0f;
    }
    fft_256(s_re, s_im);

    for (i = 1; i <= MIC_FFT_N / 2; i++) {
        float e = s_re[i] * s_re[i] + s_im[i] * s_im[i];
        s_re[i] = e;
        tot += e;
        if (i <= 8) {
            low += e;
        }
        if (e > emax) {
            emax = e;
            kmax = i;
        }
    }

    if (tot < 1.0f) {
        out->tone_x100 = 0;
        out->low_x100 = 0;
        out->f0_hz = 0;
        return;
    }

    band = s_re[kmax];
    if (kmax > 1) {
        band += s_re[kmax - 1];
    }
    if (kmax < MIC_FFT_N / 2) {
        band += s_re[kmax + 1];
    }
    {
        int tone = (int)(band * 100.0f / tot);
        int low_pct = (int)(low * 100.0f / tot);
        if (tone > 100) {
            tone = 100;
        }
        if (low_pct > 100) {
            low_pct = 100;
        }
        if (tone < 0) {
            tone = 0;
        }
        if (low_pct < 0) {
            low_pct = 0;
        }
        out->tone_x100 = (uint8_t)tone;
        out->low_x100 = (uint8_t)low_pct;
    }
    out->f0_hz = (uint16_t)(kmax * (MIC_SAMPLE_RATE / MIC_FFT_N));
}

static const char *TAG = "MIC";
static i2s_chan_handle_t s_rx;

esp_err_t MIC_Init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&chan_cfg, NULL, &s_rx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_new_channel %s", esp_err_to_name(err));
        return err;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(MIC_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = MIC_GPIO_SCK,
            .ws = MIC_GPIO_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = MIC_GPIO_SD,
        },
    };

    err = i2s_channel_init_std_mode(s_rx, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_init_std_mode %s", esp_err_to_name(err));
        i2s_del_channel(s_rx);
        s_rx = NULL;
        return err;
    }

    err = i2s_channel_enable(s_rx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable %s", esp_err_to_name(err));
        i2s_del_channel(s_rx);
        s_rx = NULL;
        return err;
    }

    ESP_LOGI(TAG, "INMP441 %u Hz  WS=%d SCK=%d SD=%d",
             (unsigned)MIC_SAMPLE_RATE, (int)MIC_GPIO_WS, (int)MIC_GPIO_SCK, (int)MIC_GPIO_SD);
    return ESP_OK;
}

static uint16_t mic_mag16(int32_t raw)
{
    int32_t s = raw >> 16;
    if (s < 0) {
        s = -s;
    }
    if (s > 32767) {
        s = 32767;
    }
    return (uint16_t)s;
}

esp_err_t MIC_ReadPeak(uint16_t *left, uint16_t *right)
{
    static int32_t buf[512];
    size_t bytes = 0;
    uint16_t peak_l = 0;
    uint16_t peak_r = 0;

    if (s_rx == NULL || left == NULL || right == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = i2s_channel_read(s_rx, buf, sizeof(buf), &bytes, 200);
    size_t samples = bytes / sizeof(int32_t);
    samples &= ~((size_t)1);

    for (size_t i = 0; i + 1 < samples; i += 2) {
        uint16_t l = mic_mag16(buf[i]);
        uint16_t r = mic_mag16(buf[i + 1]);
        if (l > peak_l) {
            peak_l = l;
        }
        if (r > peak_r) {
            peak_r = r;
        }
    }

    *left = peak_l;
    *right = peak_r;

    if (samples == 0) {
        return (err != ESP_OK) ? err : ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t MIC_ReadFrame(mic_frame_t *out)
{
    static int32_t buf[512];
    static int16_t left[MIC_FFT_N];
    static int16_t right[MIC_FFT_N];
    size_t bytes = 0;
    uint16_t peak_l = 0;
    uint16_t peak_r = 0;
    uint32_t sum_l = 0;
    uint32_t sum_r = 0;
    uint16_t n = 0;

    if (s_rx == NULL || out == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = i2s_channel_read(s_rx, buf, sizeof(buf), &bytes, 200);
    size_t samples = bytes / sizeof(int32_t);
    samples &= ~((size_t)1);
    if (samples > MIC_FFT_N * 2) {
        samples = MIC_FFT_N * 2;
    }

    for (size_t i = 0; i + 1 < samples; i += 2) {
        int32_t sl = buf[i] >> 16;
        int32_t sr = buf[i + 1] >> 16;
        uint16_t al = mic_mag16(buf[i]);
        uint16_t ar = mic_mag16(buf[i + 1]);

        if (sl > 32767) {
            sl = 32767;
        } else if (sl < -32768) {
            sl = -32768;
        }
        if (sr > 32767) {
            sr = 32767;
        } else if (sr < -32768) {
            sr = -32768;
        }
        left[n] = (int16_t)sl;
        right[n] = (int16_t)sr;
        sum_l += al;
        sum_r += ar;
        if (al > peak_l) {
            peak_l = al;
        }
        if (ar > peak_r) {
            peak_r = ar;
        }
        n++;
    }

    if (n == 0) {
        return (err != ESP_OK) ? err : ESP_ERR_TIMEOUT;
    }

    if (peak_r > peak_l) {
        out->peak = peak_r;
        out->avg = (uint16_t)(sum_r / n);
    } else {
        out->peak = peak_l;
        out->avg = (uint16_t)(sum_l / n);
    }
    out->samples = n;
    out->crest_x10 = out->avg ? (uint16_t)((uint32_t)out->peak * 10u / out->avg) : 0;
    out->tone_x100 = 0;
    out->low_x100 = 0;
    out->f0_hz = 0;
    if (n >= MIC_FFT_N) {
        mic_spectrum((peak_r > peak_l) ? right : left, out);
    }
    return ESP_OK;
}
