//
// Created by rmukhia on 13/6/22.
//
/*
 * This should work like a sliding buffer
 */

#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/semphr.h>
#include <freertos/ringbuf.h>
#include "fft_buffer.h"
#include "esp_log.h"
#include "core/dash_light.h"
#include "fft_common.h"

static const char *TAG = "FFT_BUFFER";

#if 0
#define TRACE_FUNC              ESP_LOGI(TAG, "%s [%d]", __func__, __LINE__)
#else
#define  TRACE_FUNC
#endif

typedef void (*prepare_data_t)(const void *data, size_t len);

static void __prepare_data_stereo_16bit(const void *data, size_t len);

static void __prepare_data_mono_16bit(const void *data, size_t len);

static void __prepare_data_stereo_8bit(const void *data, size_t len);

static void __prepare_data_mono_8bit(const void *data, size_t len);

static void __prepare_data_default(const void *data, size_t len);

struct {
    RingbufHandle_t queue;
    prepare_data_t prepare_data;
    fft_buffer_t temp_element;
} __fft_buf = {
    .queue = NULL,
    .prepare_data = __prepare_data_default,
    .temp_element = {
        .ptr = NULL,
        .len = 0
    },
};


fft_buffer_t *fft_buffer_make(fft_buffer_t *buf)
{
    buf->ptr = malloc(FFT_BUFFER_SIZE * sizeof(float)); // real plus complex part
    configASSERT(buf->ptr);
    buf->len = 0;
    return buf;
}

void fft_buffer_free(fft_buffer_t *buf)
{
    free(buf->ptr);
}

void fft_buffer_init()
{
    TRACE_FUNC;
    __fft_buf.queue = xQueueCreate(1, sizeof(fft_buffer_t));
    configASSERT(__fft_buf.queue != NULL);
}

void fft_buffer_deinit()
{
    TRACE_FUNC;
    vQueueDelete(__fft_buf.queue);
}


void fft_buffer_set_params()
{
    TRACE_FUNC;
    channel_mode_t mode = dashlight.pcm.channel_mode;

    switch (dashlight.pcm.block_length) {

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
            ESP_LOGE(TAG, "Unsupported block length %d", dashlight.pcm.block_length);
            __fft_buf.prepare_data = __prepare_data_default;
    }

    // reset buffer
}

bool fft_buffer_get_buf(fft_buffer_t *buf, int timeout)
{
    TRACE_FUNC;

    return pdTRUE == xQueueReceive(__fft_buf.queue, buf, pdMS_TO_TICKS(timeout));
}

void fft_buffer_print()
{
    TRACE_FUNC;
}


void fft_buffer_prepare_data(const void *data, size_t len)
{
    // Quite sure the data is in little endian
    /*
     * PCM 16 bit 2 channel data
     * L1 R1 L2 R2 L3 R3
     * should be converted to max(L1,R1) 0 max(L2,R2) 0 max(L3, R3)
     */

    TRACE_FUNC;
    __fft_buf.prepare_data(data, len);

}

static void __prepare_data_stereo_16bit(const void *data, size_t len)
{
    TRACE_FUNC;
    static fft_buffer_t buf;
    const int16_t *l, *r;


    l = data;       // left channel
    r = data + 1;   // right channel

    if (buf.ptr == NULL) {
        fft_buffer_make(&buf);
    }

    for (; (uint8_t *) l - (uint8_t *) data < len; l += 2, r += 2) {
        float v = fmaxf(*l, *r) / INT16_MAX;
        buf.ptr[buf.len++] = v;  // real part
        buf.ptr[buf.len++] = 0;  // imaginary part

        if (buf.len == FFT_BUFFER_SIZE) {
            if (uxQueueSpacesAvailable(__fft_buf.queue) == 0) {
                fft_buffer_t del_buf;
                if (pdTRUE == xQueueReceive(__fft_buf.queue,
                                            (void *) &del_buf, pdMS_TO_TICKS(20))) {
                    fft_buffer_free(&del_buf);
                }
            }
            if (pdTRUE == xQueueSend(__fft_buf.queue, (void *) &buf, pdMS_TO_TICKS(20))) {
                fft_buffer_make(&buf);
            } else {
                buf.len = 0;   // reuse the same buffer
                ESP_LOGE(TAG, "Queue send failed!");
            }

        }
    }
}

static void __prepare_data_mono_16bit(const void *data, size_t len)
{
    TRACE_FUNC;
}

static void __prepare_data_stereo_8bit(const void *data, size_t len)
{
    TRACE_FUNC;
}

static void __prepare_data_mono_8bit(const void *data, size_t len)
{
    TRACE_FUNC;
}

static void __prepare_data_default(const void *data, size_t len)
{
    TRACE_FUNC;

}