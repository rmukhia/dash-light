#ifndef __APP_CORE_H__
#define __APP_CORE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef void (* bt_app_cb_t) (uint16_t event, void *param);

typedef struct {
    uint16_t             sig;      /*!< signal to bt_app_task */
    uint16_t             event;    /*!< message event id */
    bt_app_cb_t          cb;       /*!< context switch callback */
    void                 *param;   /*!< parameter area needs to be last */
} bt_app_msg_t;

typedef void (* bt_app_copy_cb_t) (bt_app_msg_t *msg, void *p_dest, void *p_src);

bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback);

void bt_app_task_start_up(void);

void bt_app_task_shut_down(void);

void bt_recv_task_start_up(void);

void bt_recv_task_shut_down(void);

size_t write_ringbuf(const uint8_t *data, size_t size);

#endif /* __APP_CORE_H__ */
