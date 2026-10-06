#include "rtc24.h"

#define RTC24_OK       0
#define RTC24_ERROR   -1

static uint8_t dec_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4) | (value % 10U));
}

static uint8_t bcd_to_dec(uint8_t value)
{
    return (uint8_t)(((value >> 4) * 10U) + (value & 0x0FU));
}

void rtc24_cfg_setup(rtc24_cfg_t *cfg)
{
    if (cfg == 0) {
        return;
    }
    cfg->scl.port = 0;
    cfg->scl.mask = 0U;
    cfg->sda.port = 0;
    cfg->sda.mask = 0U;
    cfg->sqw.port = 0;
    cfg->sqw.mask = 0U;
    cfg->i2c_half_period_us = 5U;
    cfg->i2c_address = RTC24_DEVICE_ADDRESS;
}

int rtc24_init(rtc24_t *ctx, const rtc24_cfg_t *cfg)
{
    if ((ctx == 0) || (cfg == 0) ||
        (cfg->scl.port == 0) || (cfg->sda.port == 0) || (cfg->sqw.port == 0)) {
        return RTC24_ERROR;
    }

    ctx->sqw = cfg->sqw;
    ctx->slave_address = cfg->i2c_address;

    bm_gpio_init_input(ctx->sqw, 1U);
    if (bm_i2c_soft_init(&ctx->i2c, cfg->scl, cfg->sda, cfg->i2c_half_period_us) != BM_I2C_OK) {
        return RTC24_ERROR;
    }

    return RTC24_OK;
}

int rtc24_write_regs(rtc24_t *ctx, uint8_t reg, const uint8_t *data, uint8_t len)
{
    uint8_t buffer[66];
    uint8_t i;

    if ((ctx == 0) || (data == 0) || (len == 0U) ||
        ((uint16_t)reg + len > (RTC24_REG_RAM_END + 1U))) {
        return RTC24_ERROR;
    }

    buffer[0] = reg;
    for (i = 0U; i < len; ++i) {
        buffer[i + 1U] = data[i];
    }

    return (bm_i2c_soft_write(&ctx->i2c, ctx->slave_address, buffer, (uint16_t)len + 1U) == BM_I2C_OK)
        ? RTC24_OK : RTC24_ERROR;
}

int rtc24_read_regs(rtc24_t *ctx, uint8_t reg, uint8_t *data, uint8_t len)
{
    if ((ctx == 0) || (data == 0) || (len == 0U) ||
        ((uint16_t)reg + len > (RTC24_REG_RAM_END + 1U))) {
        return RTC24_ERROR;
    }

    return (bm_i2c_soft_write_then_read(&ctx->i2c, ctx->slave_address,
                                        &reg, 1U, data, len) == BM_I2C_OK)
        ? RTC24_OK : RTC24_ERROR;
}

int rtc24_write_reg(rtc24_t *ctx, uint8_t reg, uint8_t data)
{
    return rtc24_write_regs(ctx, reg, &data, 1U);
}

int rtc24_read_reg(rtc24_t *ctx, uint8_t reg, uint8_t *data)
{
    return rtc24_read_regs(ctx, reg, data, 1U);
}

int rtc24_check_communication(rtc24_t *ctx)
{
    uint8_t value = 0U;

    if (rtc24_write_reg(ctx, RTC24_REG_RAM_START, RTC24_DUMMY_RAM) != RTC24_OK) {
        return RTC24_ERROR;
    }
    if (rtc24_read_reg(ctx, RTC24_REG_RAM_START, &value) != RTC24_OK) {
        return RTC24_ERROR;
    }
    return (value == RTC24_DUMMY_RAM) ? RTC24_OK : RTC24_ERROR;
}

int rtc24_default_cfg(rtc24_t *ctx)
{
    uint8_t control = RTC24_CONTROL_OUT_LOW | RTC24_CONTROL_SQWE_EN | RTC24_CONTROL_RS_1HZ;

    if (rtc24_check_communication(ctx) != RTC24_OK) {
        return RTC24_ERROR;
    }
    return rtc24_write_reg(ctx, RTC24_REG_CONTROL, control);
}

int rtc24_set_time(rtc24_t *ctx, const rtc24_time_t *time)
{
    uint8_t data[3];

    if ((time == 0) || (time->second > 59U) || (time->minute > 59U) || (time->hour > 23U)) {
        return RTC24_ERROR;
    }

    data[0] = dec_to_bcd(time->second); /* CH=0 starts oscillator */
    data[1] = dec_to_bcd(time->minute);
    data[2] = dec_to_bcd(time->hour);   /* 24-hour mode */
    return rtc24_write_regs(ctx, RTC24_REG_SECONDS, data, 3U);
}

int rtc24_read_time(rtc24_t *ctx, rtc24_time_t *time)
{
    uint8_t data[3];

    if ((time == 0) || (rtc24_read_regs(ctx, RTC24_REG_SECONDS, data, 3U) != RTC24_OK)) {
        return RTC24_ERROR;
    }

    time->second = bcd_to_dec((uint8_t)(data[0] & 0x7FU));
    time->minute = bcd_to_dec((uint8_t)(data[1] & 0x7FU));
    time->hour   = bcd_to_dec((uint8_t)(data[2] & 0x3FU));
    return RTC24_OK;
}

int rtc24_set_date(rtc24_t *ctx, const rtc24_date_t *date)
{
    uint8_t data[4];

    if ((date == 0) || (date->day_of_week > 6U) ||
        (date->day < 1U) || (date->day > 31U) ||
        (date->month < 1U) || (date->month > 12U)) {
        return RTC24_ERROR;
    }

    data[0] = dec_to_bcd(date->day_of_week);
    data[1] = dec_to_bcd(date->day);
    data[2] = dec_to_bcd(date->month);
    data[3] = dec_to_bcd(date->year);
    return rtc24_write_regs(ctx, RTC24_REG_DAY, data, 4U);
}

int rtc24_read_date(rtc24_t *ctx, rtc24_date_t *date)
{
    uint8_t data[4];

    if ((date == 0) || (rtc24_read_regs(ctx, RTC24_REG_DAY, data, 4U) != RTC24_OK)) {
        return RTC24_ERROR;
    }

    date->day_of_week = (uint8_t)(data[0] & 0x07U);
    date->day         = bcd_to_dec((uint8_t)(data[1] & 0x3FU));
    date->month       = bcd_to_dec((uint8_t)(data[2] & 0x1FU));
    date->year        = bcd_to_dec(data[3]);
    return RTC24_OK;
}

int rtc24_set_datetime(rtc24_t *ctx, const rtc24_datetime_t *datetime)
{
    uint8_t data[7];

    if ((datetime == 0) ||
        (datetime->time.second > 59U) || (datetime->time.minute > 59U) || (datetime->time.hour > 23U) ||
        (datetime->date.day_of_week > 6U) ||
        (datetime->date.day < 1U) || (datetime->date.day > 31U) ||
        (datetime->date.month < 1U) || (datetime->date.month > 12U)) {
        return RTC24_ERROR;
    }

    data[0] = dec_to_bcd(datetime->time.second);
    data[1] = dec_to_bcd(datetime->time.minute);
    data[2] = dec_to_bcd(datetime->time.hour);
    data[3] = dec_to_bcd(datetime->date.day_of_week);
    data[4] = dec_to_bcd(datetime->date.day);
    data[5] = dec_to_bcd(datetime->date.month);
    data[6] = dec_to_bcd(datetime->date.year);

    return rtc24_write_regs(ctx, RTC24_REG_SECONDS, data, 7U);
}

int rtc24_read_datetime(rtc24_t *ctx, rtc24_datetime_t *datetime)
{
    uint8_t data[7];

    if ((datetime == 0) || (rtc24_read_regs(ctx, RTC24_REG_SECONDS, data, 7U) != RTC24_OK)) {
        return RTC24_ERROR;
    }

    datetime->time.second = bcd_to_dec((uint8_t)(data[0] & 0x7FU));
    datetime->time.minute = bcd_to_dec((uint8_t)(data[1] & 0x7FU));
    datetime->time.hour   = bcd_to_dec((uint8_t)(data[2] & 0x3FU));
    datetime->date.day_of_week = (uint8_t)(data[3] & 0x07U);
    datetime->date.day   = bcd_to_dec((uint8_t)(data[4] & 0x3FU));
    datetime->date.month = bcd_to_dec((uint8_t)(data[5] & 0x1FU));
    datetime->date.year  = bcd_to_dec(data[6]);
    return RTC24_OK;
}

uint8_t rtc24_get_sqw_pin(const rtc24_t *ctx)
{
    return (ctx == 0) ? 0U : bm_gpio_read(ctx->sqw);
}

const char *rtc24_day_name_short(uint8_t day_of_week)
{
    static const char *names[7] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
    return (day_of_week < 7U) ? names[day_of_week] : "---";
}
