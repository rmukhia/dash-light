//
// Created by rmukhia on 13/6/22.
//
/*
 * This should work like a sliding buffer
 */

#include <memory.h>
#include <math.h>
#include <endian.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/task.h>
#include <malloc.h>
#include <freertos/semphr.h>
#include "fft_buffer.h"
#include "fft_task.h"
#include "esp_log.h"
#include "dash_light.h"

#define FFT_BUF_SIZE (FFT_SAMPLE_SIZE * 4)
static const char *TAG = "FFT_BUFFER";

typedef void (*prepare_data_t)(void *data, size_t len);

typedef struct fft_buf_s {
    float *data[2] __attribute__((aligned(16)));
    int list_idx __attribute__((aligned(16)));
    int pos_idx[2] __attribute__((aligned(16)));
    prepare_data_t prepare_data;
    SemaphoreHandle_t buf_mutex;
} fft_buf_t;

static void __prepare_data_stereo_16bit(void *data, size_t len);
static void __prepare_data_mono_16bit(void *data, size_t len);
static void __prepare_data_stereo_8bit(void *data, size_t len);
static void __prepare_data_mono_8bit(void *data, size_t len);
static void __prepare_data_default(void *data, size_t len);


static fft_buf_t __fft_buf = {
  .data = { NULL },
  .list_idx = 0,
  .pos_idx = { 0 },
  .prepare_data = __prepare_data_default,
  .buf_mutex = NULL,
};

void fft_buffer_init()
{
    __fft_buf.data[0] = calloc(FFT_BUF_SIZE, sizeof(float));
    __fft_buf.data[1] = calloc(FFT_BUF_SIZE, sizeof(float));
    __fft_buf.buf_mutex = xSemaphoreCreateMutex();

    configASSERT(__fft_buf.buf_mutex != NULL);
}

void fft_buffer_deinit()
{
    free(__fft_buf.data[0]);
    free(__fft_buf.data[1]);
    vSemaphoreDelete(__fft_buf.buf_mutex);
}

bool fft_buffer_lock_acquire(TickType_t timeout)
{
    return xSemaphoreTake(__fft_buf.buf_mutex, timeout) == pdTRUE;
}

bool fft_buffer_lock_release()
{
    return xSemaphoreGive(__fft_buf.buf_mutex) == pdTRUE;
}

void fft_buffer_set_params(int sample_rate, channel_mode_t mode, block_length_t block_length)
{
    switch(block_length) {

        case EIGHT:
            if (mode == MONO) {
                __fft_buf.prepare_data = __prepare_data_mono_8bit;
            } else {
                __fft_buf.prepare_data = __prepare_data_stereo_8bit;
            }
            break;
        case SIXTEEN:
            if (mode == MONO) {
                __fft_buf.prepare_data = __prepare_data_mono_16bit;
            } else {
                __fft_buf.prepare_data = __prepare_data_stereo_16bit;
            }
            break;
        case FOUR:
        case TWELVE:
        default:
            ESP_LOGE(TAG, "Unsupported block length %d", block_length);
            __fft_buf.prepare_data = __prepare_data_default;
    }

    dashlight.pcm.channel_mode = mode;
    dashlight.pcm.block_length = block_length;
    dashlight.pcm.sample_rate = sample_rate;

    // reset buffer
    __fft_buf.pos_idx[0] = 0; __fft_buf.pos_idx[1] = 0;
    __fft_buf.list_idx = 0;
}

float *fft_buffer_get_buf()
{
    int curr_pos = __fft_buf.pos_idx[__fft_buf.list_idx];


    if (curr_pos < FFT_SAMPLE_SIZE * 2) {
        //ESP_LOGI(TAG, "Buffer not okay");
        return NULL;
    }

    return &__fft_buf.data[__fft_buf.list_idx][curr_pos - (FFT_SAMPLE_SIZE * 2)];
}

void fft_buffer_print()
{
    ESP_LOGI(TAG, "List index %d, Position index %d %d", __fft_buf.list_idx, __fft_buf.pos_idx[0],
             __fft_buf.pos_idx[1]);
}


void fft_buffer_prepare_data(void *data, size_t len)
{
    // Quite sure the data is in little endian
    /*
     * PCM 16 bit 2 channel data
     * L1 R1 L2 R2 L3 R3
     * should be converted to max(L1,R1) 0 max(L2,R2) 0 max(L3, R3)
     */

    __fft_buf.prepare_data(data, len);

}

static inline void __write_data(float data)
{
    int l_idx = __fft_buf.list_idx;
    int next_l_idx = (l_idx + 1) % 2;
    size_t idx;
    if ((FFT_BUF_SIZE - __fft_buf.pos_idx[l_idx]) < (FFT_SAMPLE_SIZE * 2)) {
        idx = __fft_buf.pos_idx[next_l_idx]++;
        __fft_buf.data[next_l_idx][idx] = data;
    }

    idx =__fft_buf.pos_idx[l_idx]++;
    if (idx < FFT_BUF_SIZE) {
        __fft_buf.data[l_idx][idx] = data;
    }

    if (__fft_buf.pos_idx[l_idx] > FFT_BUF_SIZE) {
        __fft_buf.list_idx = next_l_idx;
        l_idx = next_l_idx;
        next_l_idx = (l_idx + 1) % 2;
        __fft_buf.pos_idx[next_l_idx] = 0;
    }
    dashlight.buff_recv_len++;
}

static void __prepare_data_stereo_16bit(void *data, size_t len)
{
    int16_t *data_left, *data_right;
    data_left = data;
    data_right = data_left + 1;
    size_t _len = len / sizeof (int16_t);

    if (!fft_buffer_lock_acquire(pdMS_TO_TICKS(20))) {
        return;
    }

    for (size_t i =0; i < _len/2; i++) {
        __write_data(fmaxf(*data_left, *data_right));
        __write_data(0);
        data_left+=2;
        data_right+=2;
    }

    fft_buffer_lock_release();
}

static void __prepare_data_mono_16bit(void *data, size_t len)
{

}

static void __prepare_data_stereo_8bit(void *data, size_t len)
{
    int8_t *data_left, *data_right;
    data_left = data;
    data_right = data_left + 1;

    if (!fft_buffer_lock_acquire(pdMS_TO_TICKS(20))) {
        return;
    }

    for (size_t i =0; i < len/2; i++) {
        __write_data(fmaxf(*data_left, *data_right));
        __write_data(0);
        data_left+=2;
        data_right+=2;
    }

    fft_buffer_lock_release();
}

static void __prepare_data_mono_8bit(void *data, size_t len)
{

}

static void __prepare_data_default(void *data, size_t len)
{

}
