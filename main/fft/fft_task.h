//
// Created by rmukhia on 9/6/22.
//

#ifndef DASH_LIGHT_SINK_FFT_DSP_H
#define DASH_LIGHT_SINK_FFT_DSP_H

#include "esp_err.h"


void fft_task_init(void);

esp_err_t fft_task_start(void);

esp_err_t fft_task_stop(void);


#endif //DASH_LIGHT_FFT_DSP_H
