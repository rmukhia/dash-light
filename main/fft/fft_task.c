//
// Created by rmukhia on 9/6/22.
//

#include <memory.h>
#include <endian.h>
#include <math.h>
#include <dsps_fft2r.h>
#include <dsps_wind_hann.h>
#include <dsps_view.h>
#include <esp_dsp.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "fft_task.h"
#include "core/dash_light.h"
#include "fft/fft_buffer.h"
#include "fft_amplitude.h"
#include "fft_common.h"

static const char *TAG = "FFT_TASK";
#define RADIX4

static struct {
    xTaskHandle fft_task_hdl;
    float han_window[FFT_SAMPLE_SIZE];
    unsigned int cycles_taken;
    float amplitude_s[FREQ_BIN_SIZE];
} __fft_task = {
    .fft_task_hdl = NULL,
    .han_window =  {0},
    .cycles_taken = 0,
    .amplitude_s = {0},
};


static void print_fft_sample(const char *str, float *sample, size_t len)
{
    ESP_LOGI(TAG, "%s %f %f %f %f %f %f %f %f", str, sample[0], sample[1], sample[2], sample[3], sample[len - 4],
             sample[len - 3], sample[len - 2],
             sample[len - 1]);
}

static void fft_task(void *pv)
{
    ESP_LOGI(TAG, "%s started", __func__);
    TickType_t last_wakeup_time;
    TickType_t period;
    fft_buffer_t buf;
    float *amplitude_bins = NULL;

    period = pdMS_TO_TICKS(10);
    ESP_LOGI(TAG, "Period %u", period);
    last_wakeup_time = xTaskGetTickCount();

    for (;;) {
        if (!fft_buffer_get_buf(&buf, portMAX_DELAY)) {
            // we failed to get a buffer
            goto end_loop;
        }

        //dsps_mul_f32(buf.ptr, __fft_task.han_window, buf.ptr, FFT_SAMPLE_SIZE, 2, 1, 2);

#ifdef RADIX4
        dsps_fft4r_fc32(buf.ptr, FFT_SAMPLE_SIZE);
        dsps_bit_rev4r_fc32(buf.ptr, FFT_SAMPLE_SIZE);
        //dsps_cplx2reC_fc32(buf.ptr, FFT_SAMPLE_SIZE);
#else
        dsps_fft2r_fc32(buf.ptr, FFT_SAMPLE_SIZE);
        dsps_bit_rev2r_fc32(buf.ptr, FFT_SAMPLE_SIZE);
#endif
        for (size_t i = 0; i <= FFT_SAMPLE_SIZE / 2; i++) {
            __fft_task.amplitude_s[i] = hypotf(buf.ptr[i * 2], buf.ptr[i * 2 + 1]);
        }
        fft_buffer_free(&buf);


        amplitude_bins = fft_amp_get_bins(__fft_task.amplitude_s, FREQ_BIN_SIZE);

        if (!amplitude_bins) {
            ESP_LOGE(TAG, "Could not create amplitude bin.");
            goto end_loop;
        }

        fft_common_set_bands(amplitude_bins);

end_loop:
        vTaskDelayUntil(&last_wakeup_time, period);
    }
}

void fft_task_init()
{
    configASSERT(xTaskCreate(fft_task, "fft_task",
                             4096, NULL,
                             configMAX_PRIORITIES - 4, &__fft_task.fft_task_hdl));

#ifdef RADIX4
    ESP_ERROR_CHECK(dsps_fft4r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE));
#else
    ESP_ERROR_CHECK(dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE));
#endif
    dsps_wind_hann_f32((float *) __fft_task.han_window, FFT_SAMPLE_SIZE);
}

esp_err_t fft_task_start()
{

    return ESP_OK;
}

esp_err_t fft_task_stop()
{
    //esp_timer_stop(__fft_task.fft_task_timer);
    return ESP_OK;
}

void fft_test(void)
{
    static float buf[18] = {1, 0,
                            23, 0,
                            46, 0,
                            3, 0,
                            56, 0,
                            23, 0,
                            56, 0,
                            76, 0};

    static float ps[9];
    ESP_ERROR_CHECK(dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE));

    dsps_fft2r_fc32(buf, 8);
    dsps_bit_rev2r_fc32(buf, 8);
    //dsps_cplx2real_fc32(buf, 8);
    //dsps_cplx2reC_fc32(buf, 8);
    for (int i = 0; i < 9; i++) {
        printf("%d. %f  %f j\n", i, buf[i * 2], buf[i * 2 + 1]);
        ps[i] = log10f(buf[i * 2] * buf[i * 2] + buf[i * 2 + 1] * buf[i * 2 + 1]);
        printf("%d. %f\n", i, ps[i]);
    }

    dsps_view(ps, 8, 16, 10, -100, 0, '.');

    printf("\n");
}