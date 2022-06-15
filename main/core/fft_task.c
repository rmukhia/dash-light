//
// Created by rmukhia on 9/6/22.
//

#include <memory.h>
#include <endian.h>
#include <math.h>
#include <dsps_fft2r.h>
#include <dsps_wind_hann.h>
#include <dsps_view.h>
#include <dsps_fft4r.h>
#include <esp_dsp.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fft_task.h"
#include "dash_light.h"
#include "fft_buffer.h"

static const char *TAG= __FILE__;

static struct {
    xTaskHandle fft_task_hdl;
    esp_timer_handle_t fft_task_timer;
    float han_window[FFT_SAMPLE_SIZE];
    unsigned int cycles_taken;
} __fft_dsp = {
    .fft_task_hdl = NULL,
    .fft_task_timer = NULL,
};

static void fft_task_timer_cb(void *arg)
{
    xTaskNotifyGive(__fft_dsp.fft_task_hdl);
}

__attribute__((aligned(16)))
float **fft;

__attribute__((aligned(16)))
float amplitude_s[FFT_SAMPLE_SIZE/2 + 1];

__attribute__((aligned(16)))
float sum_y[FFT_SAMPLE_SIZE];

static void fft_task(void *pv)
{

    unsigned int start_b, end_b;
    float *fft;
    int cntr = 0;

    for (;;) {

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        size_t n_2 = FFT_SAMPLE_SIZE/2;

        start_b = dsp_get_cpu_cycle_count();

        if (!fft_buffer_lock_acquire(pdMS_TO_TICKS(20))) {
            goto end_loop;
        }

        fft = fft_buffer_get_buf();
        if (!fft) {
            fft_buffer_lock_release();
            goto end_loop;
        }

        dsps_fft2r_fc32(fft, FFT_SAMPLE_SIZE);
        dsps_bit_rev2r_fc32(fft, FFT_SAMPLE_SIZE);
        for (size_t i = 0; i <= n_2; i++) {
            amplitude_s[i] = hypotf(fft[i * 2], fft[i * 2 + 1]);
        }

        fft_buffer_lock_release();

        if (cntr++ == 100) {
            ESP_LOGI(TAG, "packets received from bluetooth %d processed_packets %d,cycles for fft %i",
                     dashlight.bt_recv_len / 2, dashlight.buff_recv_len, end_b - start_b);
            cntr = 0;
        }

end_loop:
        end_b = dsp_get_cpu_cycle_count();
        __fft_dsp.cycles_taken = end_b - start_b;
    }
}

esp_err_t fft_task_init()
{
    esp_timer_create_args_t timer_args = {
        .callback = fft_task_timer_cb,
        .name = "fft-periodic",
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &__fft_dsp.fft_task_timer));
    configASSERT(xTaskCreate(fft_task, "fft_task",
                             4096, NULL,
                             configMAX_PRIORITIES - 4, &__fft_dsp.fft_task_hdl));

    ESP_ERROR_CHECK(dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE));
    dsps_wind_hann_f32((float *)&__fft_dsp.han_window, FFT_SAMPLE_SIZE);

    return ESP_OK;
}

esp_err_t fft_task_start()
{

    uint64_t period = 2 * ((FFT_SAMPLE_SIZE * 1000000ull) / dashlight.pcm.sample_rate);
    ESP_LOGI(TAG, "Period %llu", period);
    //period = 100000;
    ESP_ERROR_CHECK(esp_timer_start_periodic(__fft_dsp.fft_task_timer, period));
    return ESP_OK;
}

esp_err_t fft_task_stop()
{
    esp_timer_stop(__fft_dsp.fft_task_timer);
    dsps_fft2r_deinit_fc32();
    return ESP_OK;
}

esp_err_t fft_test(void)
{
    static float buf[18] = { 1, 0,
                             23, 0,
                             46, 0,
                             3, 0,
                             56, 0,
                             23, 0,
                             56, 0,
                             76,  0};

    static float ps[9];
    ESP_ERROR_CHECK(dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE));

    dsps_fft2r_fc32(buf, 8);
    dsps_bit_rev2r_fc32(buf, 8);
    //dsps_cplx2real_fc32(buf, 8);
    for (int i = 0; i < 9; i++) {
        printf("%d. %f  %f j\n",i, buf[i*2], buf[i*2 + 1]);
        ps[i] = log10f(buf[i * 2] * buf[i* 2] + buf[i * 2 +1] * buf[i *2 + 1]);
        printf("%d. %f\n",i, ps[i]);
    }

    dsps_view(ps, 8, 16, 10, -100, 0, '.');

    printf("\n");


    return ESP_OK;
}