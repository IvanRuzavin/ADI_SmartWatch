#include "bm_i2c_soft.h"
#include "delays.h"

#define I2C_CLOCK_STRETCH_TIMEOUT_US  (1000U)

static void i2c_delay(const bm_i2c_soft_t *ctx)
{
    Delay_us(ctx->half_period_us);
}

static int scl_release_wait(const bm_i2c_soft_t *ctx)
{
    uint16_t timeout = I2C_CLOCK_STRETCH_TIMEOUT_US;
    bm_gpio_od_release(ctx->scl);
    while (!bm_gpio_read(ctx->scl)) {
        if (timeout-- == 0U) {
            return BM_I2C_BUS_STUCK;
        }
        Delay_us(1U);
    }
    return BM_I2C_OK;
}

static int i2c_start(const bm_i2c_soft_t *ctx)
{
    bm_gpio_od_release(ctx->sda);
    if (scl_release_wait(ctx) != BM_I2C_OK) {
        return BM_I2C_BUS_STUCK;
    }
    i2c_delay(ctx);

    if (!bm_gpio_read(ctx->sda)) {
        return BM_I2C_BUS_STUCK;
    }

    bm_gpio_od_low(ctx->sda);
    i2c_delay(ctx);
    bm_gpio_od_low(ctx->scl);
    i2c_delay(ctx);
    return BM_I2C_OK;
}

static void i2c_stop(const bm_i2c_soft_t *ctx)
{
    bm_gpio_od_low(ctx->sda);
    i2c_delay(ctx);
    (void)scl_release_wait(ctx);
    i2c_delay(ctx);
    bm_gpio_od_release(ctx->sda);
    i2c_delay(ctx);
}

static int i2c_write_bit(const bm_i2c_soft_t *ctx, uint8_t bit)
{
    if (bit) {
        bm_gpio_od_release(ctx->sda);
    } else {
        bm_gpio_od_low(ctx->sda);
    }
    i2c_delay(ctx);

    if (scl_release_wait(ctx) != BM_I2C_OK) {
        return BM_I2C_BUS_STUCK;
    }
    i2c_delay(ctx);
    bm_gpio_od_low(ctx->scl);
    i2c_delay(ctx);
    return BM_I2C_OK;
}

static int i2c_read_bit(const bm_i2c_soft_t *ctx, uint8_t *bit)
{
    bm_gpio_od_release(ctx->sda);
    i2c_delay(ctx);

    if (scl_release_wait(ctx) != BM_I2C_OK) {
        return BM_I2C_BUS_STUCK;
    }
    i2c_delay(ctx);
    *bit = bm_gpio_read(ctx->sda);
    bm_gpio_od_low(ctx->scl);
    i2c_delay(ctx);
    return BM_I2C_OK;
}

static int i2c_write_byte(const bm_i2c_soft_t *ctx, uint8_t value)
{
    uint8_t bit;
    uint8_t ack;

    for (bit = 0U; bit < 8U; ++bit) {
        if (i2c_write_bit(ctx, (value & 0x80U) ? 1U : 0U) != BM_I2C_OK) {
            return BM_I2C_BUS_STUCK;
        }
        value <<= 1;
    }

    if (i2c_read_bit(ctx, &ack) != BM_I2C_OK) {
        return BM_I2C_BUS_STUCK;
    }
    return ack ? BM_I2C_NACK : BM_I2C_OK;
}

static int i2c_read_byte(const bm_i2c_soft_t *ctx, uint8_t *value, uint8_t ack)
{
    uint8_t i;
    uint8_t bit;
    uint8_t result = 0U;

    for (i = 0U; i < 8U; ++i) {
        if (i2c_read_bit(ctx, &bit) != BM_I2C_OK) {
            return BM_I2C_BUS_STUCK;
        }
        result = (uint8_t)((result << 1) | bit);
    }

    /* ACK is an active-low ninth bit. */
    if (i2c_write_bit(ctx, ack ? 0U : 1U) != BM_I2C_OK) {
        return BM_I2C_BUS_STUCK;
    }

    *value = result;
    return BM_I2C_OK;
}

int bm_i2c_soft_init(bm_i2c_soft_t *ctx,
                     bm_gpio_pin_t scl,
                     bm_gpio_pin_t sda,
                     uint16_t half_period_us)
{
    uint8_t pulse;

    if ((ctx == 0) || (half_period_us == 0U)) {
        return BM_I2C_ERROR;
    }

    ctx->scl = scl;
    ctx->sda = sda;
    ctx->half_period_us = half_period_us;

    bm_gpio_init_open_drain(ctx->scl);
    bm_gpio_init_open_drain(ctx->sda);
    bm_gpio_od_release(ctx->scl);
    bm_gpio_od_release(ctx->sda);
    i2c_delay(ctx);

    /* Recover a target that was reset in the middle of a byte. */
    if (!bm_gpio_read(ctx->sda)) {
        for (pulse = 0U; pulse < 9U; ++pulse) {
            bm_gpio_od_low(ctx->scl);
            i2c_delay(ctx);
            if (scl_release_wait(ctx) != BM_I2C_OK) {
                return BM_I2C_BUS_STUCK;
            }
            i2c_delay(ctx);
        }
        i2c_stop(ctx);
    }

    return (bm_gpio_read(ctx->scl) && bm_gpio_read(ctx->sda)) ? BM_I2C_OK : BM_I2C_BUS_STUCK;
}

int bm_i2c_soft_write(bm_i2c_soft_t *ctx, uint8_t address7,
                      const uint8_t *data, uint16_t length)
{
    uint16_t i;
    int status;

    if ((ctx == 0) || ((length != 0U) && (data == 0))) {
        return BM_I2C_ERROR;
    }

    status = i2c_start(ctx);
    if (status != BM_I2C_OK) {
        return status;
    }

    status = i2c_write_byte(ctx, (uint8_t)(address7 << 1));
    if (status != BM_I2C_OK) {
        i2c_stop(ctx);
        return status;
    }

    for (i = 0U; i < length; ++i) {
        status = i2c_write_byte(ctx, data[i]);
        if (status != BM_I2C_OK) {
            i2c_stop(ctx);
            return status;
        }
    }

    i2c_stop(ctx);
    return BM_I2C_OK;
}

int bm_i2c_soft_write_then_read(bm_i2c_soft_t *ctx, uint8_t address7,
                                const uint8_t *write_data, uint16_t write_length,
                                uint8_t *read_data, uint16_t read_length)
{
    uint16_t i;
    int status;

    if ((ctx == 0) || ((write_length != 0U) && (write_data == 0)) ||
        ((read_length != 0U) && (read_data == 0))) {
        return BM_I2C_ERROR;
    }

    status = i2c_start(ctx);
    if (status != BM_I2C_OK) {
        return status;
    }

    status = i2c_write_byte(ctx, (uint8_t)(address7 << 1));
    if (status != BM_I2C_OK) {
        i2c_stop(ctx);
        return status;
    }

    for (i = 0U; i < write_length; ++i) {
        status = i2c_write_byte(ctx, write_data[i]);
        if (status != BM_I2C_OK) {
            i2c_stop(ctx);
            return status;
        }
    }

    if (read_length != 0U) {
        status = i2c_start(ctx); /* repeated START */
        if (status != BM_I2C_OK) {
            i2c_stop(ctx);
            return status;
        }

        status = i2c_write_byte(ctx, (uint8_t)((address7 << 1) | 1U));
        if (status != BM_I2C_OK) {
            i2c_stop(ctx);
            return status;
        }

        for (i = 0U; i < read_length; ++i) {
            status = i2c_read_byte(ctx, &read_data[i], (i + 1U < read_length) ? 1U : 0U);
            if (status != BM_I2C_OK) {
                i2c_stop(ctx);
                return status;
            }
        }
    }

    i2c_stop(ctx);
    return BM_I2C_OK;
}
