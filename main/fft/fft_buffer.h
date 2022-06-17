//
// Created by rmukhia on 13/6/22.
//

#ifndef TEST_APP_FFT_BUFFER_H
#define TEST_APP_FFT_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include "core/dash_light.h"

#define FFT_BUFFER_SIZE (FFT_SAMPLE_SIZE * 2)

typedef struct fft_buffer_s {
    float *ptr;
    size_t len;
} fft_buffer_t;

fft_buffer_t *fft_buffer_make(fft_buffer_t *buf);

void fft_buffer_free(fft_buffer_t *buf);

void fft_buffer_init();

void fft_buffer_deinit();

void fft_buffer_set_params();

void fft_buffer_prepare_data(const void *data, size_t len);

bool fft_buffer_get_buf(fft_buffer_t *buf, int timeout);

void fft_buffer_print();


#endif //TEST_APP_FFT_BUFFER_H
