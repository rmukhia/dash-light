/**************************************************
 * Author: rmukhia
 * Creation Date: 20/6/22
 * Description:
 **************************************************/
#include <stddef.h>
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>
#include <esp_log.h>
#include <math.h>
#include <freertos/semphr.h>
#include <memory.h>
#include <dsps_wind_hann.h>
#include "freqb.h"
#include "freqb_bands.h"

#ifdef FREQB_FFT_ALGO_RADIX4

#include <dsps_fft4r.h>

#else
#include <dsps_fft2r.h>
#endif

static const char *TAG = "FREQB";

#if 0
#define TRACE_FUNC              ESP_LOGI(TAG, "%s [%d]", __func__, __LINE__)
#else
#define  TRACE_FUNC
#endif

#define FFT_BUFFER_SIZE   (FREQB_FFT_SAMPLE_SIZE * 2)
#define FREQ_BIN_SIZE     (FREQB_FFT_SAMPLE_SIZE / 2 + 1)
#define MAX_BANDS         12

#define VERIFY_SUCCESS(stmt)    {                   \
    esp_err_t ret = stmt;                           \
    if (ret != ESP_OK) {                            \
        return ret;                                 \
    }                                               \
}

#define VERIFY_SUCCESS_SAFE(stmt, safe_stmt) {      \
    esp_err_t ret = stmt;                           \
    if (ret != ESP_OK) {                            \
        safe_stmt                                   \
        return ret;                                 \
    }                                               \
}

typedef void (*prepare_data_t)(const void *data, size_t len);

/* Holds the samples sent to fft */
typedef struct fft_buffer_s {
    float *ptr;
    size_t len;
} fft_buffer_t;

/* handler for buffer */
typedef struct buffer_hdl_s {
    QueueHandle_t q;
    prepare_data_t prepare_data;
} buffer_hdl_t;

/* handler for fft task */
typedef struct fft_task_hdl_s {
    xTaskHandle task_hdl;
    float han_window[FREQB_FFT_SAMPLE_SIZE];
} fft_task_hdl_t;

/* frequency and bin to put into */
typedef struct freq_bin_s {
    int freq;
    int bin;
} freq_bin_t;

/* handler for amplitude bands */
typedef struct amplitude_hdl_s {
    freq_bin_t freq_bin[FREQ_BIN_SIZE];
} amplitude_hdl_t;

/* handler for output, most likely db values */
typedef struct band_hdl_s {
    float val[MAX_BANDS];
    SemaphoreHandle_t val_mutex;
} band_hdl_t;

typedef enum state_e {
    UNINIT,
    INIT,
    PARAMS_SET,
    RUNNING,
} state_t;

/* */
typedef struct freqb_hdl_s {
    volatile state_t state;
    buffer_hdl_t buffer_hdl;
    fft_task_hdl_t fft_task_hdl;
    amplitude_hdl_t amplitude_hdl;
    band_hdl_t band_hdl;
    freqb_params_t params;
} freqb_hdl_t;


#define get_buf_hdl(x)      &(x).buffer_hdl;

static esp_err_t __buffer_init();

static void __buffer_deinit();

static fft_buffer_t *__buffer_make(fft_buffer_t *buf);

static void __buffer_free(fft_buffer_t *buf);

static esp_err_t __buffer_set_params(freqb_channel_mode_t mode, freqb_block_length_t blength);

static bool __buffer_get_buf(fft_buffer_t *buf, int timeout);

static void __prepare_data_stereo_16bit(const void *data, size_t len);

static void __prepare_data_mono_16bit(const void *data, size_t len);

static void __prepare_data_stereo_8bit(const void *data, size_t len);

static void __prepare_data_mono_8bit(const void *data, size_t len);

static void __prepare_data_default(const void *data, size_t len);

#define get_fft_task_hdl(x) &(x).fft_task_hdl;

static esp_err_t __fft_task_init();

static void __fft_task_deinit();

static void __fft_task(void *pv);

#define get_amp_hdl(x)      &(x).amplitude_hdl;

static esp_err_t __amp_set_params(size_t num_bands, int sample_rate);

static float *__amp_get_bins(const float *amplitude, size_t in_len);

#define get_band_hdl(x)      &(x).band_hdl;

static esp_err_t __band_init();

static void __band_deinit();

static void __band_set_val(const float *val);


static freqb_hdl_t freqb_hdl = {
    .state = UNINIT,
    .buffer_hdl = {
        .q = NULL,
        .prepare_data = __prepare_data_default,
    },
    .fft_task_hdl = {

    },
    .amplitude_hdl = {
        .freq_bin = {{.freq = 0, .bin = 0}},
    },
    .band_hdl = {
        .val = {0},
        .val_mutex = NULL,
    },
    .params = FREQB_DEFAULT_PARAMS,
};

esp_err_t __buffer_init()
{
    TRACE_FUNC;
    buffer_hdl_t *hdl = get_buf_hdl(freqb_hdl);

    hdl->q = xQueueCreate(1, sizeof(fft_buffer_t));
    return hdl->q != NULL ? ESP_OK : ESP_ERR_NO_MEM;
}

void __buffer_deinit()
{
    TRACE_FUNC;
    buffer_hdl_t *hdl = get_buf_hdl(freqb_hdl);

    vQueueDelete(hdl->q);
}

fft_buffer_t *__buffer_make(fft_buffer_t *buf)
{
    buf->ptr = malloc(FFT_BUFFER_SIZE * sizeof(float)); // real plus complex part
    configASSERT(buf->ptr);
    buf->len = 0;
    return buf;
}

void __buffer_free(fft_buffer_t *buf)
{
    free(buf->ptr);
}

esp_err_t __buffer_set_params(freqb_channel_mode_t mode, freqb_block_length_t blength)
{
    TRACE_FUNC;
    buffer_hdl_t *hdl = get_buf_hdl(freqb_hdl);

    switch (blength) {

        case FREQB_BL_EIGHT:
            if (mode == FREQB_CM_MONO) {
                hdl->prepare_data = __prepare_data_mono_8bit;
            } else {
                hdl->prepare_data = __prepare_data_stereo_8bit;
            }
            break;
        case FREQB_BL_SIXTEEN:
            if (mode == FREQB_CM_MONO) {
                hdl->prepare_data = __prepare_data_mono_16bit;
            } else {
                hdl->prepare_data = __prepare_data_stereo_16bit;
            }
            break;
        case FREQB_BL_FOUR:
        case FREQB_BL_TWELVE:
        default:
            ESP_LOGE(TAG, "Unsupported block length %d", blength);
            hdl->prepare_data = __prepare_data_default;
            return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

bool __buffer_get_buf(fft_buffer_t *buf, int timeout)
{
    TRACE_FUNC;
    buffer_hdl_t *hdl = get_buf_hdl(freqb_hdl);

    return pdTRUE == xQueueReceive(hdl->q, buf, pdMS_TO_TICKS(timeout));
}


static fft_buffer_t __tmp_buf = {
    .ptr = NULL,
    .len = 0,
};

void __prepare_data_stereo_16bit(const void *data, size_t len)
{
    buffer_hdl_t *hdl = get_buf_hdl(freqb_hdl);

    const int16_t *l, *r;


    l = data;       // left channel
    r = data + 1;   // right channel

    if (__tmp_buf.ptr == NULL) {
        __buffer_make(&__tmp_buf);
    }

    for (; (uint8_t *) l - (uint8_t *) data < len; l += 2, r += 2) {
        float v = fmaxf(*l, *r) / INT16_MAX;
        __tmp_buf.ptr[__tmp_buf.len++] = v;  // real part
        __tmp_buf.ptr[__tmp_buf.len++] = 0;  // imaginary part

        if (__tmp_buf.len == FFT_BUFFER_SIZE) {
            if (uxQueueSpacesAvailable(hdl->q) == 0) {
                fft_buffer_t del_buf;
                if (pdTRUE == xQueueReceive(hdl->q,
                                            (void *) &del_buf, pdMS_TO_TICKS(20))) {
                    __buffer_free(&del_buf);
                }
            }
            if (pdTRUE == xQueueSend(hdl->q, (void *) &__tmp_buf, pdMS_TO_TICKS(20))) {
                __buffer_make(&__tmp_buf);
            } else {
                __tmp_buf.len = 0;   // reuse the same buffer
                ESP_LOGE(TAG, "Queue send failed!");
            }

        }
    }
}

void __prepare_data_mono_16bit(const void *data, size_t len)
{
}

void __prepare_data_stereo_8bit(const void *data, size_t len)
{
}

void __prepare_data_mono_8bit(const void *data, size_t len)
{
}

void __prepare_data_default(const void *data, size_t len)
{

}

esp_err_t __amp_set_params(size_t num_bands, int sample_rate)
{
    TRACE_FUNC;
    amplitude_hdl_t *hdl = get_amp_hdl(freqb_hdl);
    int current_band = 0;
    const int *band_freq = freqb_get_num_bands(num_bands);


    for (size_t i = 0; i < FREQ_BIN_SIZE; i++) {
        hdl->freq_bin[i].freq = (int) (i * sample_rate / (float) FREQB_FFT_SAMPLE_SIZE);
    }

    for (size_t i = 1; i < FREQ_BIN_SIZE; i++) {
        hdl->freq_bin[i].bin = current_band;
        if (current_band != (num_bands - 1) && hdl->freq_bin[i].freq > band_freq[current_band]) {
            current_band++;
            printf("%i  %i\n", hdl->freq_bin[i].freq, current_band);
        }
    }

    return ESP_OK;
}

static float __amp_bands[MAX_BANDS] = {0};

float *__amp_get_bins(const float *amplitude, size_t in_len)
{
    amplitude_hdl_t *hdl = get_amp_hdl(freqb_hdl);
    int current_band = -1;

    if (in_len != FREQ_BIN_SIZE) {
        ESP_LOGE(TAG, "FFT bins mismatch. %d %d", in_len, FREQ_BIN_SIZE);
        return NULL;
    }

    for (size_t n = 1; n < in_len; n++) {
        if (current_band != hdl->freq_bin[n].bin) {
            __amp_bands[hdl->freq_bin[n].bin] = amplitude[n];
            current_band = hdl->freq_bin[n].bin;
        } else {
            // get max amplitude
            if (__amp_bands[hdl->freq_bin[n].bin] < amplitude[n]) {
                __amp_bands[hdl->freq_bin[n].bin] = amplitude[n];
            }
        }

    }

    return __amp_bands;
}

esp_err_t __band_init()
{
    TRACE_FUNC;
    band_hdl_t *hdl = get_band_hdl(freqb_hdl);

    hdl->val_mutex = xSemaphoreCreateMutex();

    return hdl->val_mutex != NULL ? ESP_OK : ESP_ERR_NO_MEM;
}

void __band_deinit()
{
    TRACE_FUNC;
    band_hdl_t *hdl = get_band_hdl(freqb_hdl);

    vSemaphoreDelete(hdl->val_mutex);
}

void __band_set_val(const float *val)
{
    band_hdl_t *hdl = get_band_hdl(freqb_hdl);
    //(num_bands == freqb.bands.num);

    if (xSemaphoreTake(hdl->val_mutex, pdMS_TO_TICKS(20))) {
        for (size_t n = 0; n < freqb_hdl.params.num_bands; n++) {
            //freqb.bands.db[n] = (20.f * log10(val[n]/ 0.78));
            hdl->val[n] = val[n];
        }
        xSemaphoreGive(hdl->val_mutex);
    }
}

esp_err_t __fft_task_init()
{
    TRACE_FUNC;
    fft_task_hdl_t *hdl = get_fft_task_hdl(freqb_hdl);


#ifdef FREQB_FFT_ALGO_RADIX4
    VERIFY_SUCCESS(dsps_fft4r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE));
#else
    VERIFY_SUCCESS(dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE));
#endif
    dsps_wind_hann_f32((float *) hdl->han_window, FREQB_FFT_SAMPLE_SIZE);

    return ESP_OK;
}

void __fft_task_deinit()
{
    TRACE_FUNC;

    dsps_fft4r_deinit_fc32();
}


void __print_fft_sample(const char *str, float *sample, size_t len)
{
    ESP_LOGI(TAG, "%s:[%d] %f %f %f %f %f %f %f %f", str, len, sample[0], sample[1], sample[2], sample[3], sample[len - 4],
             sample[len - 3], sample[len - 2],
             sample[len - 1]);
}

static float amplitude[FREQ_BIN_SIZE] = {0};

void __fft_task(void *pv)
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
        if (!__buffer_get_buf(&buf, portMAX_DELAY)) {
            // we failed to get a buffer
            goto end_loop;
        }

        //dsps_mul_f32(buf.ptr, __fft_task.han_window, buf.ptr, FFT_SAMPLE_SIZE, 2, 1, 2);

        //__print_fft_sample("FFT", buf.ptr, buf.len);

#ifdef FREQB_FFT_ALGO_RADIX4
        dsps_fft4r_fc32(buf.ptr, FREQB_FFT_SAMPLE_SIZE);
        dsps_bit_rev4r_fc32(buf.ptr, FREQB_FFT_SAMPLE_SIZE);
        //dsps_cplx2reC_fc32(buf.ptr, FFT_SAMPLE_SIZE);
#else
        dsps_fft2r_fc32(buf.ptr, FREQB_FFT_SAMPLE_SIZE);
        dsps_bit_rev2r_fc32(buf.ptr, FREQB_FFT_SAMPLE_SIZE);
#endif
        for (size_t i = 0; i <= FREQB_FFT_SAMPLE_SIZE / 2; i++) {
            amplitude[i] = hypotf(buf.ptr[i * 2], buf.ptr[i * 2 + 1]);
        }
        __buffer_free(&buf);


        amplitude_bins = __amp_get_bins(amplitude, FREQ_BIN_SIZE);

        if (!amplitude_bins) {
            ESP_LOGE(TAG, "Could not create amplitude bin.");
            goto end_loop;
        }

        __band_set_val(amplitude_bins);

end_loop:
        vTaskDelayUntil(&last_wakeup_time, period);
    }
}


esp_err_t freqb_init()
{
    TRACE_FUNC;

    if (freqb_hdl.state != UNINIT) {
        return ESP_ERR_INVALID_STATE;
    }

    VERIFY_SUCCESS(__band_init());

    VERIFY_SUCCESS_SAFE(__buffer_init(), {
        __band_deinit();
    });

    VERIFY_SUCCESS_SAFE(__fft_task_init(), {
        __buffer_deinit();
        __band_deinit();
    });

    freqb_hdl.state = INIT;

    return ESP_OK;
}

void freqb_deinit()
{
    TRACE_FUNC;

    if (freqb_hdl.state == UNINIT) {
        return;
    }
    __fft_task_deinit();
    __buffer_deinit();
    __band_deinit();

    freqb_hdl.state = UNINIT;
}

esp_err_t freqb_set_params(freqb_params_t params)
{
    TRACE_FUNC;

    if (freqb_hdl.state == UNINIT) {
        return ESP_ERR_INVALID_STATE;
    }
    freqb_hdl.params = params;

    ESP_ERROR_CHECK(__buffer_set_params(params.channel_mode, params.block_length));

    ESP_ERROR_CHECK(__amp_set_params(params.num_bands, params.sample_rate));

    freqb_hdl.state = PARAMS_SET;

    return ESP_OK;
}

esp_err_t freqb_start()
{
    TRACE_FUNC;
    fft_task_hdl_t *fft_task_hdl = get_fft_task_hdl(freqb_hdl);

    if (freqb_hdl.state != PARAMS_SET) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xTaskCreate(__fft_task, "fft_task",
                    4096, NULL,
                    configMAX_PRIORITIES - 4,
                    &fft_task_hdl->task_hdl) == pdFALSE) {
        return ESP_ERR_NO_MEM;
    }

    freqb_hdl.state = RUNNING;
    return ESP_OK;
}

esp_err_t freqb_stop()
{
    TRACE_FUNC;
    fft_task_hdl_t *fft_task_hdl = get_fft_task_hdl(freqb_hdl);

    if (freqb_hdl.state != RUNNING) {
        return ESP_ERR_INVALID_STATE;
    }

    vTaskDelete(fft_task_hdl->task_hdl);

    freqb_hdl.state = PARAMS_SET;
    return ESP_OK;
}

void freqb_prepare_data(const void *data, size_t len)
{
    TRACE_FUNC;
    buffer_hdl_t *hdl = get_buf_hdl(freqb_hdl);

    if (freqb_hdl.state == RUNNING) {
        hdl->prepare_data(data, len);
    }
    // Quite sure the data is in little endian
    /*
     * PCM 16 bit 2 channel data
     * L1 R1 L2 R2 L3 R3
     * should be converted to max(L1,R1) 0 max(L2,R2) 0 max(L3, R3)
     */
}

esp_err_t freqb_get_val(float *val, size_t *num_bands, uint32_t timeout)
{
    band_hdl_t *hdl = get_band_hdl(freqb_hdl);

    if (freqb_hdl.state != RUNNING) {
        return ESP_ERR_INVALID_STATE;
    }

    *num_bands = freqb_hdl.params.num_bands;

    if (*num_bands == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(hdl->val_mutex, timeout) == pdTRUE) {
        memcpy(val, hdl->val, *num_bands * sizeof (float));
        xSemaphoreGive(hdl->val_mutex);
    } else {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

