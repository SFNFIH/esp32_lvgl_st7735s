#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lv_port_disp.h"

void app_main(void)
{
    lv_init();
    lv_port_disp_init();
    while (1)
    {
        lv_task_handler();
        lv_tick_inc(1);
        vTaskDelay(1);
        
    }
}
