//
// Created by rmukhia on 9/6/22.
//

#ifndef DASH_LIGHT_H
#define DASH_LIGHT_H

#include <stddef.h>

typedef enum channel_mode_e {
    MONO= 0x01,
    DUAL_CHANNEL = 0x02,
    STEREO = 0x02,
    JOINT_STEREO = 0x02,
} channel_mode_t;

typedef enum block_length_e {
    FOUR = 4,
    EIGHT = 8,
    TWELVE = 12,
    SIXTEEN = 16
} block_length_t;

typedef struct dashlight_s {
    size_t bt_recv_len;
    size_t buff_recv_len;
    struct {
        int sample_rate;
        channel_mode_t channel_mode;
        block_length_t block_length;
    } pcm;
} dashlight_t;


extern dashlight_t dashlight;

#endif //DASH_LIGHT_H
