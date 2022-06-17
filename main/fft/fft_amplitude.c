//
// Created by rmukhia on 15/6/22.
//
#include <esp_log.h>
#include <stdio.h>
#include <math.h>
#include "fft_common.h"
#include "fft_amplitude.h"
#include "core/dash_light.h"

static const char *TAG = "FFT_DB";


static struct {
    int frequency_bins[FREQ_BIN_SIZE];
    int bins[FREQ_BIN_SIZE];
    float amp_bands[DB_MAX_BANDS];      // amplitude bands
} __fft_amp = {
    .frequency_bins = {0},
    .bins = {0},
};

static const int band_3_freq[] = {120, 1000, 16000};
static const int band_4_freq[] = {120, 800, 2000, 16000};
static const int band_10_freq[] = {60, 120, 250, 355, 710, 1420, 2840, 5680, 11360, 16000};

/*
 * Geometric progression summation, with r = 2
 */
#define FIRST_ITEM(n, g)    ceil(-n/(1.f - (pow(2, (g - 1)))))
#define NEXT_ITEM(i)        i * 2

void fft_amp_set_params()
{
    int *band_freq = NULL;
    int current_band = 0;


    switch (fft.bands.num) {
        case 3:
            band_freq = (int *) band_3_freq;
            break;
        case 4:
            band_freq = (int *) band_4_freq;
            break;
        case 10:
        default:
            band_freq = (int *) band_10_freq;
            break;
    }

    for (size_t n = 0; n < FREQ_BIN_SIZE; n++) {
        __fft_amp.frequency_bins[n] = n * dashlight.pcm.sample_rate / (float) FFT_SAMPLE_SIZE;
    }


    for (size_t n = 1; n < FREQ_BIN_SIZE; n++) {
        __fft_amp.bins[n] = current_band;
        if (current_band != (fft.bands.num - 1) && __fft_amp.frequency_bins[n] > band_freq[current_band]) {
            current_band++;
            printf("%i  %i\n", __fft_amp.frequency_bins[n], current_band);
        }
    }
}

float *fft_amp_get_bins(const float *amplitude, size_t in_len)
{
    int current_band = -1;

    if (in_len != FREQ_BIN_SIZE) {
        ESP_LOGE(TAG, "FFT bins mismatch. %d %d", in_len, FREQ_BIN_SIZE);
        return NULL;
    }

    for (size_t n = 1; n < in_len; n++) {
        if (current_band != __fft_amp.bins[n]) {
            __fft_amp.amp_bands[__fft_amp.bins[n]] = amplitude[n];
            current_band = __fft_amp.bins[n];
        } else {
            // get max amplitude
            if (__fft_amp.amp_bands[__fft_amp.bins[n]] < amplitude[n]) {
                __fft_amp.amp_bands[__fft_amp.bins[n]] = amplitude[n];
            }
        }

    }

    return __fft_amp.amp_bands;
}
