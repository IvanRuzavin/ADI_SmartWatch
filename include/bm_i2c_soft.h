#ifndef BM_I2C_SOFT_H
#define BM_I2C_SOFT_H

#include <stdint.h>
#include "bm_gpio.h"

typedef struct {
    bm_gpio_pin_t scl;
    bm_gpio_pin_t sda;
    uint16_t half_period_us;
} bm_i2c_soft_t;

typedef enum {
    BM_I2C_OK = 0,
    BM_I2C_ERROR = -1,
    BM_I2C_NACK = -2,
    BM_I2C_BUS_STUCK = -3
} bm_i2c_status_t;

int bm_i2c_soft_init(bm_i2c_soft_t *ctx,
                     bm_gpio_pin_t scl,
                     bm_gpio_pin_t sda,
                     uint16_t half_period_us);
int bm_i2c_soft_write(bm_i2c_soft_t *ctx, uint8_t address7,
                      const uint8_t *data, uint16_t length);
int bm_i2c_soft_write_then_read(bm_i2c_soft_t *ctx, uint8_t address7,
                                const uint8_t *write_data, uint16_t write_length,
                                uint8_t *read_data, uint16_t read_length);

#endif /* BM_I2C_SOFT_H */
