#ifndef EQUIPMENT_H
#define EQUIPMENT_H

typedef enum {
    LED_ON,
    LED_OFF
} led_state_t;

void dsp_init();

void ws2812_set_color(uint32_t index, uint32_t red, uint32_t green, uint32_t blue);

void ws2812_set_livel(led_state_t sw);
#endif
