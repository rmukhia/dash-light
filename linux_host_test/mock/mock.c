/*******************************************************************************
* Author: Raunak Mukhia @rmukhia
* Date:   30/01/22
*
* File:  mock.c
* Descr:
*******************************************************************************/

#include <stdio.h>
#include <malloc.h>
#include <esp_event.h>
#include "nvs.h"

#define MOCK_RET_OK(dec)        dec { return ESP_OK; }

MOCK_RET_OK(esp_err_t  nvs_flash_init());
MOCK_RET_OK(esp_err_t  nvs_flash_erase());
MOCK_RET_OK(esp_err_t nvs_open(const char *tag, int mode, nvs_handle_t *hdl));
MOCK_RET_OK(esp_err_t nvs_set_blob(nvs_handle_t hdl, const char *key, const void *data, const size_t len));
MOCK_RET_OK(esp_err_t nvs_get_blob(nvs_handle_t hdl, const char *key, void *data, size_t *len));
MOCK_RET_OK(esp_err_t nvs_close(nvs_handle_t hdl));
MOCK_RET_OK(esp_err_t nvs_commit(nvs_handle_t hdl));
MOCK_RET_OK(esp_err_t nvs_erase_key(nvs_handle_t hdl, const char *data));
MOCK_RET_OK(esp_err_t nvs_erase_all(nvs_handle_t hdl));


MOCK_RET_OK(esp_err_t esp_event_handler_instance_register(esp_event_base_t base, int event, esp_event_handler_t handler,
                                                          void * args, esp_event_handler_instance_t instance));


MOCK_RET_OK(esp_err_t esp_event_post(esp_event_base_t event_base,
                         int32_t event_id,
                         void *event_data,
                         size_t event_data_size,
                         TickType_t ticks_to_wait));