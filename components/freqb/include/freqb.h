/**************************************************
 * Author: rmukhia
 * Creation Date: 20/6/22
 * Description: 
 **************************************************/

#ifndef FREQB_H
#define FREQB_H

#define FREQB_FFT_SAMPLE_SIZE   1024
#define FREQB_FFT_ALGO_RADIX4
//#define FREQB_FFT_ALGO_RADIX2

typedef enum freq_channel_mode_e {
    FREQB_CM_MONO           = 0x01,
    FREQB_CM_DUAL_CHANNEL   = 0x02,
    FREQB_CM_STEREO         = 0x02,
    FREQB_CM_JOINT_STEREO   = 0x02,
} freqb_channel_mode_t;

typedef enum freqb_block_length_e {
    FREQB_BL_FOUR           = 4,
    FREQB_BL_EIGHT          = 8,
    FREQB_BL_TWELVE         = 12,
    FREQB_BL_SIXTEEN        = 16
} freqb_block_length_t;

typedef struct freqb_params_s {
    freqb_channel_mode_t channel_mode;
    freqb_block_length_t block_length;
    int sample_rate;
    size_t num_bands;
} freqb_params_t;

/* default parameters */
#define FREQB_DEFAULT_PARAMS {          \
    .channel_mode = FREQB_CM_STEREO,    \
    .block_length = FREQB_BL_SIXTEEN,   \
    .sample_rate = 44100,               \
    .num_bands = 0                      \
}



esp_err_t freqb_init();

void freqb_deinit();

esp_err_t freqb_set_params(freqb_params_t params);

esp_err_t freqb_start();

esp_err_t freqb_stop();

/* Get the band array of length num_bands */
esp_err_t freqb_get_val(float *val, size_t *num_bands, uint32_t timeout);

/* Send PCM data to the freqb component */
void freqb_prepare_data(const void *data, size_t len);

#endif //FREQB_H
