//
// Created by rmukhia on 16/6/22.
//

#ifndef LIGHT_BURST_FFT_COMMON_H
#define LIGHT_BURST_FFT_COMMON_H

#include <stddef.h>

#define FFT_SAMPLE_SIZE                 1024
#define FFT_SHIFT_TIME                  1000 //ms

#define DB_MAX_BANDS                    12

typedef struct fft_common_s {
    struct {
        float db[DB_MAX_BANDS];
        size_t num;
    } bands;
} fft_common_t;


void fft_test(void);

void fft_init();

void fft_common_set_bands(const float *amplitude_bins);

bool fft_bands_lock_acquire(int timeout);

bool fft_bands_lock_release();

float *fft_common_get_bands(size_t *num_bands);

extern fft_common_t fft;

#endif //LIGHT_BURST_FFT_COMMON_H
