#ifndef BM_GPIO_H
#define BM_GPIO_H

#include <stdint.h>
#include "mcu.h"

typedef struct {
    mxc_gpio_regs_t *port;
    uint32_t mask;
} bm_gpio_pin_t;

void bm_gpio_init_output(bm_gpio_pin_t pin, uint8_t initial_high);
void bm_gpio_init_input(bm_gpio_pin_t pin, uint8_t pull_up);
void bm_gpio_init_open_drain(bm_gpio_pin_t pin);
void bm_gpio_init_alt1(bm_gpio_pin_t pin);
void bm_gpio_init_alt2(bm_gpio_pin_t pin);
void bm_gpio_write(bm_gpio_pin_t pin, uint8_t high);
void bm_gpio_set(bm_gpio_pin_t pin);
void bm_gpio_clear(bm_gpio_pin_t pin);
uint8_t bm_gpio_read(bm_gpio_pin_t pin);
void bm_gpio_od_low(bm_gpio_pin_t pin);
void bm_gpio_od_release(bm_gpio_pin_t pin);

#endif /* BM_GPIO_H */
