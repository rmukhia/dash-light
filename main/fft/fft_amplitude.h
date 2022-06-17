//
// Created by rmukhia on 15/6/22.
//

#ifndef LIGHT_BURST_FFT_AMPLITUDE_H
#define LIGHT_BURST_FFT_AMPLITUDE_H

#include <stddef.h>

#define FREQ_BIN_SIZE       (FFT_SAMPLE_SIZE / 2 + 1)

void fft_amp_set_params();

float *fft_amp_get_bins(const float *amplitude, size_t in_len);

#endif //LIGHT_BURST_FFT_AMPLITUDE_H
