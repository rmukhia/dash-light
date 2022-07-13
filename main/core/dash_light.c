//
// Created by rmukhia on 9/6/22.
//

#include "dash_light.h"

dashlight_t dashlight = {
    .bt_recv_len  = 0,
    .buff_recv_len = 0,
    .pcm = {
        .sample_rate = 441000,
        .channel_mode = STEREO,
        .block_length = SIXTEEN,
    },
};

