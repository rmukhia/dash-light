//
// Created by rmukhia on 13/6/22.
//

#ifndef TEST_APP_FFT_BUFFER_H
#define TEST_APP_FFT_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include "dash_light.h"

void fft_buffer_init();
void fft_buffer_deinit();

bool fft_buffer_lock_acquire(TickType_t timeout);
bool fft_buffer_lock_release();

void fft_buffer_set_params(int sample_rate, channel_mode_t mode, block_length_t block_length);
void fft_buffer_prepare_data(void *data, size_t len);
float *fft_buffer_get_buf();

void fft_buffer_print();

#endif //TEST_APP_FFT_BUFFER_H
