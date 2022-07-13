//
// Created by rmukhia on 18/6/22.
//

#include <esp_err.h>
#include <esp_log.h>
#include "oled.h"
#include "driver/gpio.h"
#include "u8g2_esp32_hal.h"

static const char *TAG = "DISPLAY_OLED";

#define OLED_SDA    GPIO_NUM_21
#define OLED_SCL    GPIO_NUM_22

#define SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128xt32
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64

static struct {
    QueueHandle_t cmd_q;
    TaskHandle_t display_task_hdl;
    uint8_t i2c_address;
    struct {
        size_t width;
        size_t height;
    } shape;
    u8g2_esp32_hal_t u8g2_esp32_hal;
    u8g2_t driver;
} __oled = {
    .cmd_q = NULL,
    .display_task_hdl = NULL,
    .i2c_address = SCREEN_ADDRESS,
    .shape = {
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
    },
    .u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT,
};

static void screen_text_draw(u8g2_t *hw)
{
    ESP_LOGE(TAG, "About Menu");
    u8g2_ClearBuffer(hw);
    u8g2_SetPowerSave(hw, 0); // wake up display

    u8g2_DrawBox(hw, 0, 26, 80,6);
    u8g2_DrawFrame(hw, 0,26,100,6);
    u8g2_SetFont(hw, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(hw, 2,17, "Raunak Mukhia");

    u8g2_SendBuffer(hw);
}

void __display_eq(display_param_eq_t *eq_param)
{
    //eq_param->bands;
    u8g2_ClearBuffer(&__oled.driver);
    size_t w = __oled.shape.width - 10;
    size_t b = w/eq_param->num_bands - 2;
    for (size_t i = 0; i < eq_param->num_bands; i++) {
        ESP_LOGI(TAG, "x %d y %d", 5 + (i * b), 50);
        u8g2_DrawHLine(&__oled.driver, 5 + (i * b), 50, i + 10);

    }
    u8g2_SendBuffer(&__oled.driver);
}

void __display_task(void *pv)
{
    display_cmd_t cmd;

    ESP_LOGI(TAG, "__display_task started");

    u8g2_SetPowerSave(&__oled.driver, 0);
    screen_text_draw(&__oled.driver);

    for(;;) {
        if (xQueueReceive(__oled.cmd_q, &cmd, pdMS_TO_TICKS(portMAX_DELAY)) == pdFALSE){
            continue;
        }

        switch(cmd.mode) {

            case DISPLAY_MODE_NONE:
                break;
            case DISPLAY_MODE_MENU:
                break;
            case DISPLAY_MODE_EQ:
                __display_eq(&cmd.eq_params);
                break;
        }

    }
}

esp_err_t display_oled_init()
{
    __oled.u8g2_esp32_hal.sda = OLED_SDA;
    __oled.u8g2_esp32_hal.scl = OLED_SCL;

    u8g2_esp32_hal_init(__oled.u8g2_esp32_hal);

    u8g2_Setup_ssd1306_i2c_128x64_vcomh0_f(
        &__oled.driver,
        U8G2_R0,		//u8x8_byte_sw_i2c,
        u8g2_esp32_i2c_byte_cb,
        u8g2_esp32_gpio_and_delay_cb);  // init u8g2 structure

    u8x8_SetI2CAddress(&__oled.driver.u8x8, __oled.i2c_address << 1);

    ESP_LOGI(TAG, "u8g2_InitDisplay");
    // send init sequence to the display, display is in sleep mode after this,
    u8g2_InitDisplay(&__oled.driver);

    ESP_LOGI(TAG, "Initialized OLED driver %p",  &__oled.driver);

    __oled.cmd_q = xQueueCreate(1, sizeof(display_cmd_t));

    if (__oled.cmd_q == NULL) {
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(__display_task, "__display_task",
                    2048, NULL,
                    configMAX_PRIORITIES - 10, &__oled.display_task_hdl) == pdFALSE) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t display_oled_update(display_cmd_t *cmd)
{
    xQueueOverwrite(__oled.cmd_q, cmd);
    return ESP_OK;
}