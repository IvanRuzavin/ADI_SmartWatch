#ifndef RTC24_BARE_METAL_H
#define RTC24_BARE_METAL_H

#include <stdint.h>
#include "bm_gpio.h"
#include "bm_i2c_soft.h"

#define RTC24_REG_SECONDS               0x00U
#define RTC24_REG_MINUTES               0x01U
#define RTC24_REG_HOURS                 0x02U
#define RTC24_REG_DAY                   0x03U
#define RTC24_REG_DATE                  0x04U
#define RTC24_REG_MONTH                 0x05U
#define RTC24_REG_YEAR                  0x06U
#define RTC24_REG_CONTROL               0x07U
#define RTC24_REG_RAM_START             0x08U
#define RTC24_REG_RAM_END               0x3FU

#define RTC24_CONTROL_OUT_LOW           0x00U
#define RTC24_CONTROL_OUT_HIGH          0x80U
#define RTC24_CONTROL_SQWE_DIS          0x00U
#define RTC24_CONTROL_SQWE_EN           0x10U
#define RTC24_CONTROL_RS_1HZ            0x00U
#define RTC24_CONTROL_RS_4_096KHZ       0x01U
#define RTC24_CONTROL_RS_8_192KHZ       0x02U
#define RTC24_CONTROL_RS_32_768KHZ      0x03U

/* Same weekday encoding as the supplied RTC 24 Click package. */
#define RTC24_MONDAY                    0U
#define RTC24_TUESDAY                   1U
#define RTC24_WEDNESDAY                 2U
#define RTC24_THURSDAY                  3U
#define RTC24_FRIDAY                    4U
#define RTC24_SATURDAY                  5U
#define RTC24_SUNDAY                    6U

#define RTC24_DEVICE_ADDRESS            0x68U
#define RTC24_DUMMY_RAM                 0xA5U

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
} rtc24_time_t;

typedef struct {
    uint8_t day_of_week;
    uint8_t day;
    uint8_t month;
    uint8_t year;       /* 00..99 means 2000..2099 in this application */
} rtc24_date_t;

typedef struct {
    rtc24_time_t time;
    rtc24_date_t date;
} rtc24_datetime_t;

typedef struct {
    bm_gpio_pin_t scl;
    bm_gpio_pin_t sda;
    bm_gpio_pin_t sqw;
    uint16_t i2c_half_period_us;
    uint8_t i2c_address;
} rtc24_cfg_t;

typedef struct {
    bm_i2c_soft_t i2c;
    bm_gpio_pin_t sqw;
    uint8_t slave_address;
} rtc24_t;

void rtc24_cfg_setup(rtc24_cfg_t *cfg);
int rtc24_init(rtc24_t *ctx, const rtc24_cfg_t *cfg);
int rtc24_default_cfg(rtc24_t *ctx);
int rtc24_check_communication(rtc24_t *ctx);
int rtc24_write_reg(rtc24_t *ctx, uint8_t reg, uint8_t data);
int rtc24_read_reg(rtc24_t *ctx, uint8_t reg, uint8_t *data);
int rtc24_write_regs(rtc24_t *ctx, uint8_t reg, const uint8_t *data, uint8_t len);
int rtc24_read_regs(rtc24_t *ctx, uint8_t reg, uint8_t *data, uint8_t len);
int rtc24_set_time(rtc24_t *ctx, const rtc24_time_t *time);
int rtc24_read_time(rtc24_t *ctx, rtc24_time_t *time);
int rtc24_set_date(rtc24_t *ctx, const rtc24_date_t *date);
int rtc24_read_date(rtc24_t *ctx, rtc24_date_t *date);
int rtc24_set_datetime(rtc24_t *ctx, const rtc24_datetime_t *datetime);
int rtc24_read_datetime(rtc24_t *ctx, rtc24_datetime_t *datetime);
uint8_t rtc24_get_sqw_pin(const rtc24_t *ctx);
const char *rtc24_day_name_short(uint8_t day_of_week);

#endif /* RTC24_BARE_METAL_H */
