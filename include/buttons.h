#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include "bm_gpio.h"

enum {
    BUTTON_B1 = (1U << 0),
    BUTTON_B2 = (1U << 1),
    BUTTON_B3 = (1U << 2),
    BUTTON_B4 = (1U << 3),
    BUTTON_B5 = (1U << 4),
    BUTTON_B6 = (1U << 5)
};

typedef struct {
    bm_gpio_pin_t pin;
    uint8_t stable_pressed;
    uint8_t last_sample_pressed;
    uint8_t debounce_count;
    uint8_t event_mask;
    uint16_t hold_ticks;
} button_state_t;

typedef struct {
    button_state_t buttons[6];
} buttons_t;

void buttons_init(buttons_t *ctx);
uint8_t buttons_poll(buttons_t *ctx);

#endif /* BUTTONS_H */
