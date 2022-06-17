//
// Created by rmukhia on 16/6/22.
//

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <math.h>
#include "fft_common.h"
#include "fft_buffer.h"
#include "fft_task.h"

#define NUM_DB_BANDS                    10

fft_common_t fft = {
    .bands = {
        .db =  {0},
        .num = NUM_DB_BANDS,
    }
};

static struct {
    SemaphoreHandle_t bands_mutex;
} __fft = {
    .bands_mutex = NULL,
};

void fft_init()
{
    __fft.bands_mutex = xSemaphoreCreateMutex();
    configASSERT(__fft.bands_mutex != NULL);

    fft_buffer_init();
    fft_task_init();
}

bool fft_bands_lock_acquire(int timeout)
{
    return xSemaphoreTake(__fft.bands_mutex, timeout) == pdTRUE;
}

bool fft_bands_lock_release()
{
    return xSemaphoreGive(__fft.bands_mutex) == pdTRUE;
}

float *fft_common_get_bands(size_t *num_bands)
{
    *num_bands = fft.bands.num;
    return fft.bands.db;
}

void fft_common_set_bands(const float *amplitude_bins)
{
    //(num_bands == fft.bands.num);

    if (fft_bands_lock_acquire(pdMS_TO_TICKS(20))) {
        for (size_t n = 0; n < fft.bands.num; n++) {
            //fft.bands.db[n] = (20.f * log10(amplitude_bins[n]/ 0.78));
            fft.bands.db[n] = amplitude_bins[n];
        }

        fft_bands_lock_release();
    }
}