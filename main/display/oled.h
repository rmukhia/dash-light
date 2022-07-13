//
// Created by rmukhia on 18/6/22.
//

#ifndef LIGHT_BURST_OLED_H
#define LIGHT_BURST_OLED_H
#include "freqb.h"

typedef enum display_mode_e {
    DISPLAY_MODE_NONE,
    DISPLAY_MODE_MENU,
    DISPLAY_MODE_EQ,
} display_mode_t;

typedef struct display_params_eq_s {
    int num_bands;
    float bands[FREQB_MAX_BANDS];
} display_param_eq_t;

typedef struct display_param_s {
    display_mode_t mode;
    union {
        display_param_eq_t eq_params;
    };
} display_cmd_t;

esp_err_t display_oled_init();

esp_err_t display_oled_update(display_cmd_t *cmd);


#endif //LIGHT_BURST_OLED_H
