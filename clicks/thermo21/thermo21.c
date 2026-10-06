#include "thermo21.h"
#include "bm_spi.h"
#include "board_config.h"

#define THERMO21_DUMMY       (0x00U)
#define THERMO21_RESOLUTION  (32.0f)

void thermo21_cfg_setup(thermo21_t *ctx)
{
    if (ctx == 0) return;
    ctx->cs.port = BOARD_THERMO_CS_PORT;
    ctx->cs.mask = BOARD_THERMO_CS_MASK;
    ctx->spi_hz = BOARD_THERMO_SPI_HZ;
}

thermo21_status_t thermo21_init(thermo21_t *ctx)
{
    if (ctx == 0) return THERMO21_ERROR;

    /* CS is independent GPIO exactly like the supplied Click driver. */
    bm_gpio_init_output(ctx->cs, 1U);
    if (bm_spi0_init(ctx->spi_hz) != BM_SPI_OK) return THERMO21_ERROR;
    return THERMO21_OK;
}

thermo21_status_t thermo21_read_raw(thermo21_t *ctx, uint8_t raw[2])
{
    bm_spi_status_t status;
    if ((ctx == 0) || (raw == 0)) return THERMO21_ERROR;

    bm_gpio_clear(ctx->cs);
    status = bm_spi0_read(raw, 2U, THERMO21_DUMMY);
    bm_gpio_set(ctx->cs);

    return (status == BM_SPI_OK) ? THERMO21_OK : THERMO21_ERROR;
}

thermo21_status_t thermo21_temperature_read(thermo21_t *ctx, float *temperature_c)
{
    uint8_t raw[2];
    int16_t value;

    if (temperature_c == 0) return THERMO21_ERROR;
    if (thermo21_read_raw(ctx, raw) != THERMO21_OK) return THERMO21_ERROR;

    /* Same ADT7301 conversion used by the supplied Thermo 21 package. */
    value = (int16_t)(((int16_t)raw[0] << 10) | ((int16_t)raw[1] << 2));
    value >>= 2;
    *temperature_c = (float)value / THERMO21_RESOLUTION;
    return THERMO21_OK;
}

thermo21_status_t thermo21_temperature_deci_c(thermo21_t *ctx, int16_t *temperature_deci_c)
{
    uint8_t raw[2];
    int16_t value;
    int32_t scaled;

    if (temperature_deci_c == 0) return THERMO21_ERROR;
    if (thermo21_read_raw(ctx, raw) != THERMO21_OK) return THERMO21_ERROR;

    value = (int16_t)(((int16_t)raw[0] << 10) | ((int16_t)raw[1] << 2));
    value >>= 2; /* signed 14-bit value, 1/32 C per LSB */

    /* Round to nearest 0.1 C without pulling float printf into firmware. */
    scaled = (int32_t)value * 10;
    if (scaled >= 0) scaled += 16;
    else scaled -= 16;
    *temperature_deci_c = (int16_t)(scaled / 32);
    return THERMO21_OK;
}
