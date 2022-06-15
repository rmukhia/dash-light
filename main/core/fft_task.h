//
// Created by rmukhia on 9/6/22.
//

#ifndef DASH_LIGHT_SINK_FFT_DSP_H
#define DASH_LIGHT_SINK_FFT_DSP_H
#include "esp_err.h"

#define FFT_SAMPLE_SIZE                 512
#define FFT_SHIFT_TIME                  1000 //ms

esp_err_t fft_task_init(void);
esp_err_t fft_task_start(void);
esp_err_t fft_task_stop(void);

esp_err_t fft_test(void);


#endif //DASH_LIGHT_FFT_DSP_H
