/**************************************************
 * Author: rmukhia
 * Creation Date: 21/6/22
 * Description: 
 **************************************************/

#ifndef LIGHT_BURST_WIFI_MQTT_H
#define LIGHT_BURST_WIFI_MQTT_H

#include "freqb.h"

typedef enum wifi_mqtt_cmd_type_e {
    WIFI_MQTT_CMD_SEND_FREQB,
} wifi_mqtt_cmd_type_t;

typedef struct wifi_mqtt_param_sendfreqb_s {
    size_t num_bands;
    float val[FREQB_MAX_BANDS];
} wifi_mqtt_param_sendfreqb_t;

typedef struct wifi_mqtt_cmd_s {
    wifi_mqtt_cmd_type_t type;
    union {
        wifi_mqtt_param_sendfreqb_t sendfreqb;
    };
} wifi_mqtt_cmd_t;

void wifi_init_sta(void);

esp_err_t wifi_mqtt_put_cmd(wifi_mqtt_cmd_t  *cmd);


#endif //LIGHT_BURST_WIFI_MQTT_H