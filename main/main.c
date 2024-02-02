#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "esp_lcd_backlight.h"
#include "esp_freertos_hooks.h"
#include "esp_log.h"
#include "gui_guider.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "gui.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (13) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

disp_backlight_h bckl_handle;
lv_obj_t * led3;
lv_ui guider_ui;

lv_obj_t *screen_1;

LV_IMG_DECLARE(_Fan_alpha_50x50);
LV_IMG_DECLARE(_desk_lamp_alpha_50x50);

void blk_init()
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void disp_blk_set(int b)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, b * 40));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(portTICK_PERIOD_MS);
}

void lvgl_TaskCode(void *pvParameters)
{
    while (1)
    {
        lv_task_handler();
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{

    lv_init();
    lv_port_disp_init(LV_DISP_ROT_270);
    lv_port_indev_init();
    blk_init();
    setup_ui(&guider_ui);

    // gui();
    // gui_demo();
    esp_register_freertos_tick_hook((void *)lv_tick_task);
    xTaskCreate(lvgl_TaskCode, "lvgl_loop", 4096 * 4, NULL, 5, NULL);
    while (1)
    {
        vTaskDelay(10);
    }
}
