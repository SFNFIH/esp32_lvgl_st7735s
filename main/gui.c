#include "gui.h"
#include "lvgl.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "led_strip.h"

// GPIO assignment
#define LED_STRIP_BLINK_GPIO  0
// Numbers of the LED in the strip
#define LED_STRIP_LED_NUMBERS 32
// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

#define kitchen_light GPIO_NUM_12
#define Livingroom_lights GPIO_NUM_13

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_CHANNEL_1 LEDC_CHANNEL_1
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY (0)                   // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY (4000)           // Frequency in Hertz. Set frequency at 4 kHz

LV_FONT_DECLARE(lv_customer_font_SmileySans_30);

led_strip_handle_t led_strip;

lv_obj_t *background;

lv_obj_t *btn;
lv_obj_t *label;
lv_obj_t *led;

lv_obj_t *btn1;
lv_obj_t *label1;
lv_obj_t *led1;

lv_obj_t *btn2;
lv_obj_t *label2;
lv_obj_t *led2;

lv_obj_t *btn3;

lv_obj_t *weather;
lv_obj_t *tem;
lv_obj_t *thm;
lv_obj_t *timer;

lv_obj_t *wifi_led;


led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_BLINK_GPIO,   // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_NUMBERS,        // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .rmt_channel = 0,
#else
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .flags.with_dma = false,               // DMA feature is available on ESP target like ESP32-S3
#endif
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI("WS2812", "Created LED strip object with RMT backend");
    return led_strip;
}

void light_init()
{
    led_strip = configure_led();
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = kitchen_light,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ledc_channel_config_t ledc_channel1 = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = Livingroom_lights,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel1));
}

void set_kitchen_light_brightness(uint32_t brightness)
{
    if (brightness <= 100 || brightness >= 0)
    {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, brightness * 82));
        // Update duty to apply the new value
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        lv_led_set_brightness(led,(int)brightness);
    }
    else
    {
        ESP_LOGI("set_kitchen_light_brightness", "所设置亮度超过最大值");
    }
}

void set_Livingroom_lights_brightness(uint32_t brightness)
{
    if (brightness <= 100 || brightness >= 0)
    {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, brightness * 82));
        // Update duty to apply the new value
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1));
        lv_led_set_brightness(led1,(int)brightness);
    }
    else
    {
        ESP_LOGI("set_Livingroom_lights_brightness", "所设置亮度超过最大值");
    }
}

void set_kitchen_light_toggle()
{
    if(ledc_get_duty(LEDC_MODE,LEDC_CHANNEL) != 0)
    {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0));
        // Update duty to apply the new value
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        lv_led_off(led);
        for (int i = 16; i < LED_STRIP_LED_NUMBERS; i++) {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 255, 255));
        }
        /* Refresh the strip to send data */
        ESP_ERROR_CHECK(led_strip_clear(led_strip));
        ESP_LOGI("set_kitchen_light_toggle", "LED OFF!");
    }else{
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 8192));
        // Update duty to apply the new value
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        lv_led_on(led);
        for (int i = 16; i < LED_STRIP_LED_NUMBERS; i++) {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 255, 255));
        }
        /* Refresh the strip to send data */
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        ESP_LOGI("set_kitchen_light_toggle", "LED ON!");
    }
}

void set_Livingroom_lights_toggle()
{
    if(ledc_get_duty(LEDC_MODE,LEDC_CHANNEL_1) != 0)
    {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, 0));
        // Update duty to apply the new value
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1));
        lv_led_off(led1);
        for (int i = 0; i < 15; i++) {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 0, 255));
        }
        /* Refresh the strip to send data */
        ESP_ERROR_CHECK(led_strip_clear(led_strip));
        ESP_LOGI("set_Livingroom_lights_toggle", "LED OFF!");
    }else{
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, 8192));
        // Update duty to apply the new value
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1));
        lv_led_on(led1);
        for (int i = 0; i < 15; i++) {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 0, 255));
        }
        /* Refresh the strip to send data */
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        ESP_LOGI("set_Livingroom_lights_toggle", "LED ON!");
    }
}

int get_kitchen_light_brightness()
{
    return ledc_get_duty(LEDC_MODE,LEDC_CHANNEL) / 81;
}

int get_Livingroom_lights_brightness()
{
    return ledc_get_duty(LEDC_MODE,LEDC_CHANNEL_1) / 81;
}

static void event_cd(lv_event_t * e)
{
    lv_obj_t *target = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if(target == btn)
    {
        set_kitchen_light_toggle();
        if(code == LV_EVENT_CLICKED)
        {
            
        }else if(code == LV_EVENT_LONG_PRESSED){

        }
    }else if(target == btn1)
    {
        if(code == LV_EVENT_CLICKED)
        {
            set_Livingroom_lights_toggle();
        }else if(code == LV_EVENT_LONG_PRESSED){

        }
    }else if(target == btn2)
    {
        if(code == LV_EVENT_CLICKED)
        {
            lv_led_toggle(led2);
            set_Livingroom_lights_toggle();
            set_kitchen_light_toggle();
        }else if(code == LV_EVENT_LONG_PRESSED){

        }
    }

}

void gui(void)
{
    /*����:1*/
    light_init();

    // background = lv_obj_create(NULL);
	// lv_obj_set_size(background, 480, 320);

	// //Write style for screen_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	// lv_obj_set_style_bg_opa(background, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	// lv_obj_set_style_bg_color(background, lv_color_hex(0x31b7fd), LV_PART_MAIN|LV_STATE_DEFAULT);

    btn = lv_obj_create(lv_scr_act());
    lv_obj_set_size(btn,120,320);
    lv_obj_align(btn,LV_ALIGN_LEFT_MID,0,0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFBC40), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn,lv_color_hex(0xa66c00),LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn,2,LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn,event_cd,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(btn,event_cd,LV_EVENT_LONG_PRESSED,NULL);

    label = lv_label_create(btn);
    lv_obj_set_style_text_font(label,&lv_customer_font_SmileySans_30,LV_STATE_DEFAULT);
    lv_label_set_text(label,"厨房");
    //lv_obj_set_style_text_font(label,&lv_font_montserrat_20,LV_STATE_DEFAULT);
    lv_obj_align(label,LV_ALIGN_CENTER,0,0);
    lv_obj_set_style_text_color(label,lv_color_hex(0xffffff),LV_STATE_DEFAULT);

    led  = lv_led_create(btn);
    lv_led_set_brightness(led, 150);
    lv_obj_set_size(led,30,10);
    lv_obj_align(led,LV_ALIGN_TOP_MID,0,5);
    lv_led_set_color(led,lv_color_hex(0xffffff));
    lv_led_off(led);

    /*����:2*/
    btn1 = lv_obj_create(lv_scr_act());
    lv_obj_set_size(btn1,120,160);
    lv_obj_align_to(btn1,btn,LV_ALIGN_OUT_RIGHT_TOP,0,0);
    lv_obj_add_event_cb(btn1,event_cd,LV_EVENT_CLICKED,NULL);
    lv_obj_set_style_bg_color(btn1, lv_color_hex(0xffa500), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn1,lv_color_hex(0xa66c00),LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn1,2,LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn1,event_cd,LV_EVENT_LONG_PRESSED,NULL);

    label1 = lv_label_create(btn1);
    lv_obj_set_style_text_font(label1,&lv_customer_font_SmileySans_30,LV_STATE_DEFAULT);
    lv_label_set_text(label1,"卧室");
    lv_obj_align(label1,LV_ALIGN_CENTER,0,30);
    lv_obj_set_style_text_color(label1,lv_color_hex(0xffffff),LV_STATE_DEFAULT);

    led1  = lv_led_create(btn1);
    lv_led_set_brightness(led1, 150);
    lv_obj_set_size(led1,30,10);
    lv_obj_align(led1,LV_ALIGN_TOP_MID,0,5);
    lv_led_set_color(led1,lv_color_hex(0xffffff));
    lv_led_off(led1);

    /*����:3*/
    btn2 = lv_obj_create(lv_scr_act());
    lv_obj_set_size(btn2,120,160);
    lv_obj_align_to(btn2,btn1,LV_ALIGN_OUT_BOTTOM_MID,0,0);
    lv_obj_add_event_cb(btn2,event_cd,LV_EVENT_CLICKED,NULL);
    lv_obj_set_style_bg_color(btn2, lv_color_hex(0xffce73), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn2,lv_color_hex(0xa66c00),LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn2,2,LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn2,event_cd,LV_EVENT_LONG_PRESSED,NULL);

    label2 = lv_label_create(btn2);
    lv_obj_set_style_text_font(label2,&lv_customer_font_SmileySans_30,LV_STATE_DEFAULT);
    lv_label_set_text(label2,"客厅");
    lv_obj_align(label2,LV_ALIGN_CENTER,0,30);
    lv_obj_set_style_text_color(label2,lv_color_hex(0xffffff),LV_STATE_DEFAULT);

    led2  = lv_led_create(btn2);
    lv_led_set_brightness(led2, 150);
    lv_obj_set_size(led2,30,10);
    lv_obj_align(led2,LV_ALIGN_TOP_MID,0,5);
    lv_led_set_color(led2,lv_color_hex(0xffffff));

    /*����ʱ��ģ��*/
    btn3 = lv_obj_create(lv_scr_act());
    lv_obj_set_size(btn3,240,320);
    lv_obj_align(btn3,LV_ALIGN_RIGHT_MID,0,0);
    lv_obj_add_event_cb(btn3,event_cd,LV_EVENT_CLICKED,NULL);
    lv_obj_set_style_bg_color(btn3, lv_color_hex(0xffcF00), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn3,lv_color_hex(0xa66c00),LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn3,2,LV_STATE_DEFAULT);

    weather = lv_label_create(btn3);
    lv_label_set_text_fmt(weather,"天气: %s","晴天");
    lv_obj_set_style_text_font(weather,&lv_customer_font_SmileySans_30,LV_STATE_DEFAULT);
    // lv_label_set_long_mode(weather, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_size(weather,200,50);
    lv_obj_align(weather,LV_ALIGN_TOP_MID,0,0);
    lv_obj_set_style_text_color(weather,lv_color_hex(0xffffff),LV_STATE_DEFAULT);

    tem = lv_label_create(btn3);
    lv_label_set_text_fmt(tem,"温度: %d°",26);
    lv_obj_set_style_text_font(tem,&lv_customer_font_SmileySans_30,LV_STATE_DEFAULT);
    lv_obj_align(tem,LV_ALIGN_RIGHT_MID,0,-50);
    lv_obj_set_size(tem,140,80);
    lv_obj_set_style_text_color(tem,lv_color_hex(0xffffff),LV_STATE_DEFAULT);

    thm = lv_label_create(btn3);
    lv_label_set_text_fmt(thm,"湿度: %d%%",66);
    lv_obj_set_style_text_font(thm,&lv_customer_font_SmileySans_30,LV_STATE_DEFAULT);
    lv_obj_align(thm,LV_ALIGN_RIGHT_MID,0,-15);
    lv_obj_set_size(thm,150,80);
    // lv_obj_set_style_text_font(thm,&lv_font_montserrat_20,LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(thm,lv_color_hex(0xffffff),LV_STATE_DEFAULT);

    timer= lv_label_create(btn3);
    lv_label_set_text_fmt(timer,"%d : %d",10,26);
    lv_obj_align(timer,LV_ALIGN_CENTER,0,0);
    lv_obj_set_style_text_font(timer,&lv_font_montserrat_30,LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(timer,lv_color_hex(0xffffff),LV_STATE_DEFAULT);

    wifi_led = lv_led_create(btn3);
    lv_obj_align(wifi_led,LV_ALIGN_RIGHT_MID,0,-135);
    lv_obj_set_size(wifi_led,20,20);
}

static void anim_x_cb(void * var, int32_t v)
{
    lv_obj_set_y(var, v);
}

static void sw_event_cb(lv_event_t * e)
{
    lv_obj_t * sw = lv_event_get_target(e);
    lv_obj_t * label = lv_event_get_user_data(e);

    if(lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, label);
        lv_anim_set_values(&a, 200, 50);
        lv_anim_set_time(&a, 500);
        lv_anim_set_exec_cb(&a, anim_x_cb);
        lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
        lv_anim_start(&a);
    }
    else {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, label);
        lv_anim_set_values(&a, 50, 200);
        lv_anim_set_time(&a, 500);
        lv_anim_set_exec_cb(&a, anim_x_cb);
        lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
        lv_anim_start(&a);
    }

}

void gui_demo(void)
{
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello animations!");
    lv_obj_set_pos(label, 250, 10);


    lv_obj_t * sw = lv_switch_create(lv_scr_act());
    lv_obj_center(sw);
    lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, sw_event_cb, LV_EVENT_VALUE_CHANGED, label);
}

