/**************************************************
 * Author: rmukhia
 * Creation Date: 21/6/22
 * Description: 
 **************************************************/

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_event_base.h>
#include <esp_wifi_types.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_wifi_default.h>
#include <esp_wifi.h>
#include <mqtt_client.h>
#include "wifi_mqtt.h"

static const char *TAG = "WIFI_MQTT";

#define MQTT_FREQB_TOPIC       "/dashlight/" CONFIG_MQTT_ID "/freqb"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

typedef enum wifi_mqtt_state_e {
    WIFI_MQTT_UNINIT,
    WIFI_MQTT_INIT,
    WIFI_MQTT_CONNECTED,
} wifi_mqtt_state_t;

struct {
    wifi_mqtt_state_t state;
    struct {
        EventGroupHandle_t evt_grp;
        int retry_num;
    } wifi;
    struct {
        esp_mqtt_client_handle_t client;
        TaskHandle_t cmd_task_hdl;
        QueueHandle_t cmd_q;
    } mqtt;
} __wifi_mqtt = {
    .state = WIFI_MQTT_UNINIT,
    .wifi = {
        .evt_grp = NULL,
        .retry_num = 0,
    },
    .mqtt = {
        .cmd_task_hdl = NULL,
        .cmd_q = NULL,
    },
};

static void __cmd_sendfreqb(wifi_mqtt_cmd_t *cmd)
{
    static char json[512];
    char buf[16];
    memset(json, 0, sizeof (json));
    strcpy(json, "{ ");
    for (size_t i =0; i < cmd->sendfreqb.num_bands; i++) {
        snprintf(buf, 16, " \"%i\" : %.3f ", i, cmd->sendfreqb.val[i]);
        strcat(json, buf);
        if (i < cmd->sendfreqb.num_bands - 1) {
            strcat(json, ",");
        }
    }
    strcat(json, " }");

    if (esp_mqtt_client_publish(__wifi_mqtt.mqtt.client, MQTT_FREQB_TOPIC, json, 0, 0, 0) == -1)
    {
        ESP_LOGE(TAG, "MQTT send failed for %s", json);
    }

}

static void __cmd_task(void *pv)
{
    wifi_mqtt_cmd_t cmd;
    for (;;) {
        if (xQueueReceive(__wifi_mqtt.mqtt.cmd_q, &cmd, portMAX_DELAY) ==pdFALSE) {
            continue;
        }

        switch (cmd.type) {

            case WIFI_MQTT_CMD_SEND_FREQB:
                __cmd_sendfreqb(&cmd);
                break;
        }
    }

}

static void __log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void __mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t) event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            __wifi_mqtt.state = WIFI_MQTT_CONNECTED;

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
/*
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                __log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                __log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                __log_error_if_nonzero("captured as transport's socket errno",
                                       event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

            }
            */
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void __mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };

    __wifi_mqtt.mqtt.client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example __mqtt_event_handler */
    esp_mqtt_client_register_event(__wifi_mqtt.mqtt.client, ESP_EVENT_ANY_ID, __mqtt_event_handler, NULL);
    esp_mqtt_client_start(__wifi_mqtt.mqtt.client);
}


static void __wifi_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (__wifi_mqtt.wifi.retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            __wifi_mqtt.wifi.retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(__wifi_mqtt.wifi.evt_grp, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        __wifi_mqtt.wifi.retry_num = 0;
        xEventGroupSetBits(__wifi_mqtt.wifi.evt_grp, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    __wifi_mqtt.wifi.evt_grp = xEventGroupCreate();

    configASSERT(xTaskCreate(__cmd_task, "__cmd_task", 2048,
                             NULL, configMAX_PRIORITIES - 10,
                             &__wifi_mqtt.mqtt.cmd_task_hdl) == pdTRUE);

    __wifi_mqtt.mqtt.cmd_q = xQueueCreate(1, sizeof (wifi_mqtt_cmd_t));

    configASSERT(__wifi_mqtt.mqtt.cmd_q != NULL);

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &__wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &__wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_ESP_WIFI_SSID,
            .password = CONFIG_ESP_WIFI_PASSWORD,
            .threshold.authmode =  WIFI_AUTH_OPEN,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(__wifi_mqtt.wifi.evt_grp,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        __wifi_mqtt.state = WIFI_MQTT_INIT;
        __mqtt_init();
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

esp_err_t wifi_mqtt_put_cmd(wifi_mqtt_cmd_t *cmd)
{
    if (__wifi_mqtt.state == WIFI_MQTT_CONNECTED) {
        xQueueOverwrite(__wifi_mqtt.mqtt.cmd_q, cmd);
    }

    return ESP_ERR_INVALID_STATE;
}