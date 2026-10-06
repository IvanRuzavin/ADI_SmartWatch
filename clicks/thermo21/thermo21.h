#ifndef THERMO21_BAREMETAL_H
#define THERMO21_BAREMETAL_H

#include <stdint.h>
#include "bm_gpio.h"

typedef enum {
    THERMO21_OK = 0,
    THERMO21_ERROR = -1
} thermo21_status_t;

typedef struct {
    bm_gpio_pin_t cs;
    uint32_t spi_hz;
} thermo21_t;

void thermo21_cfg_setup(thermo21_t *ctx);
thermo21_status_t thermo21_init(thermo21_t *ctx);
thermo21_status_t thermo21_read_raw(thermo21_t *ctx, uint8_t raw[2]);
thermo21_status_t thermo21_temperature_read(thermo21_t *ctx, float *temperature_c);
thermo21_status_t thermo21_temperature_deci_c(thermo21_t *ctx, int16_t *temperature_deci_c);

#endif /* THERMO21_BAREMETAL_H */
