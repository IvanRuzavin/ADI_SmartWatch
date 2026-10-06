/**
 * MAX32690 bare-metal multi-screen dashboard:
 *   - IPS Display 2 Click on mikroBUS 1
 *   - Oximeter 5 Click on mikroBUS 2
 *   - Thermo 21 Click on mikroBUS 3
 *   - RTC 24 Click on mikroBUS 4
 *   - six local buttons
 *
 * Platform dependencies remain intentionally limited to mcu.h and delays.h.
 */
#include "mcu.h"
#include "delays.h"
#include "board_config.h"
#include "buttons.h"
#include "ips_display2.h"
#include "rtc24.h"
#include "thermo21.h"
#include "oximeter5.h"
#include "clock_ui.h"

#ifdef PREINIT_SUPPORTED
extern void preinit(void);
#endif

#define RTC24_OK 0

static rtc24_t rtc;
static thermo21_t thermo;
static oximeter5_t oximeter;
static buttons_t buttons;

static rtc24_datetime_t live_datetime;
static rtc24_datetime_t previous_datetime;
static clock_ui_state_t ui;
static environment_data_t environment;
static environment_data_t previous_environment;
static oximeter5_data_t oximeter_data;
static oximeter5_data_t previous_oximeter_data;
static oximeter5_data_t rendered_oximeter_data;

static app_screen_t active_screen = APP_SCREEN_CLOCK;
static uint8_t rtc_available = 0U;
static uint8_t thermo_available = 0U;
static uint8_t oximeter_available = 0U;

static uint8_t days_in_month(uint8_t month, uint8_t year)
{
    static const uint8_t days[12] = { 31U,28U,31U,30U,31U,30U,31U,31U,30U,31U,30U,31U };
    uint8_t result;

    if ((month < 1U) || (month > 12U)) return 31U;
    result = days[month - 1U];
    if ((month == 2U) && ((year % 4U) == 0U)) result = 29U;
    return result;
}

static void clamp_day(rtc24_datetime_t *dt)
{
    uint8_t max_day = days_in_month(dt->date.month, dt->date.year);
    if (dt->date.day < 1U) dt->date.day = 1U;
    if (dt->date.day > max_day) dt->date.day = max_day;
}

static void change_selected_value(int8_t direction)
{
    rtc24_datetime_t *dt = &ui.edit_value;

    switch (ui.field) {
        case CLOCK_FIELD_HOUR:
            if (direction > 0) dt->time.hour = (uint8_t)((dt->time.hour + 1U) % 24U);
            else dt->time.hour = (dt->time.hour == 0U) ? 23U : (uint8_t)(dt->time.hour - 1U);
            break;
        case CLOCK_FIELD_MINUTE:
            if (direction > 0) dt->time.minute = (uint8_t)((dt->time.minute + 1U) % 60U);
            else dt->time.minute = (dt->time.minute == 0U) ? 59U : (uint8_t)(dt->time.minute - 1U);
            break;
        case CLOCK_FIELD_SECOND:
            if (direction > 0) dt->time.second = (uint8_t)((dt->time.second + 1U) % 60U);
            else dt->time.second = (dt->time.second == 0U) ? 59U : (uint8_t)(dt->time.second - 1U);
            break;
        case CLOCK_FIELD_MONTH:
            if (direction > 0) dt->date.month = (dt->date.month >= 12U) ? 1U : (uint8_t)(dt->date.month + 1U);
            else dt->date.month = (dt->date.month <= 1U) ? 12U : (uint8_t)(dt->date.month - 1U);
            clamp_day(dt);
            break;
        case CLOCK_FIELD_DAY: {
            uint8_t max_day = days_in_month(dt->date.month, dt->date.year);
            if (direction > 0) dt->date.day = (dt->date.day >= max_day) ? 1U : (uint8_t)(dt->date.day + 1U);
            else dt->date.day = (dt->date.day <= 1U) ? max_day : (uint8_t)(dt->date.day - 1U);
            break;
        }
        case CLOCK_FIELD_YEAR:
            if (direction > 0) dt->date.year = (uint8_t)((dt->date.year + 1U) % 100U);
            else dt->date.year = (dt->date.year == 0U) ? 99U : (uint8_t)(dt->date.year - 1U);
            clamp_day(dt);
            break;
        case CLOCK_FIELD_WEEKDAY:
            if (direction > 0) dt->date.day_of_week = (uint8_t)((dt->date.day_of_week + 1U) % 7U);
            else dt->date.day_of_week = (dt->date.day_of_week == 0U) ? 6U : (uint8_t)(dt->date.day_of_week - 1U);
            break;
        default:
            break;
    }
}

static void draw_active_screen(void)
{
    switch (active_screen) {
        case APP_SCREEN_CLOCK:
            clock_ui_show_main(&live_datetime, rtc_available, &environment, &oximeter_data);
            break;
        case APP_SCREEN_CLIMATE:
            clock_ui_show_climate(&environment);
            break;
        case APP_SCREEN_OXIMETER:
            clock_ui_show_oximeter(&oximeter_data);
            rendered_oximeter_data = oximeter_data;
            break;
        default:
            active_screen = APP_SCREEN_CLOCK;
            clock_ui_show_main(&live_datetime, rtc_available, &environment, &oximeter_data);
            break;
    }
}

static void next_screen(void)
{
    active_screen = (app_screen_t)(((uint8_t)active_screen + 1U) % (uint8_t)APP_SCREEN_COUNT);
    draw_active_screen();
}

static void enter_editor(void)
{
    ui.mode = CLOCK_UI_NAVIGATE;
    ui.field = CLOCK_FIELD_HOUR;
    ui.blink_on = 1U;
    ui.edit_value = live_datetime;
    clamp_day(&ui.edit_value);
    clock_ui_show_editor(&ui);
}

static void save_and_exit_editor(void)
{
    if (rtc_available && (rtc24_set_datetime(&rtc, &ui.edit_value) == RTC24_OK)) {
        live_datetime = ui.edit_value;
        previous_datetime = live_datetime;
    }
    ui.mode = CLOCK_UI_VIEW;
    active_screen = APP_SCREEN_CLOCK;
    clock_ui_show_main(&live_datetime, rtc_available, &environment, &oximeter_data);
}

static void process_editor_buttons(uint8_t events)
{
    if (events & BUTTON_B1) {
        save_and_exit_editor();
        return;
    }

    if (ui.mode == CLOCK_UI_NAVIGATE) {
        if (events & BUTTON_B2) {
            clock_ui_field_t old_field = ui.field;
            ui.field = (clock_ui_field_t)(((uint8_t)ui.field + 1U) % (uint8_t)CLOCK_FIELD_COUNT);
            ui.blink_on = 1U;
            clock_ui_redraw_field(&ui, old_field);
            clock_ui_redraw_field(&ui, ui.field);
        }
        if (events & BUTTON_B3) {
            ui.mode = CLOCK_UI_EDIT_VALUE;
            ui.blink_on = 1U;
            clock_ui_show_editor(&ui);
        }
    } else if (ui.mode == CLOCK_UI_EDIT_VALUE) {
        if (events & BUTTON_B2) {
            ui.mode = CLOCK_UI_NAVIGATE;
            ui.blink_on = 1U;
            clock_ui_show_editor(&ui);
            return;
        }
        /* buttons_poll() auto-repeats B4/B5 after a 1-second hold. */
        if (events & BUTTON_B4) {
            change_selected_value(+1);
            clock_ui_redraw_field(&ui, ui.field);
        }
        if (events & BUTTON_B5) {
            change_selected_value(-1);
            clock_ui_redraw_field(&ui, ui.field);
        }
    }
}

static void poll_thermo21(void)
{
    int16_t value;
    previous_environment = environment;

    if (thermo21_temperature_deci_c(&thermo, &value) == THERMO21_OK) {
        environment.temperature_valid = 1U;
        environment.temperature_deci_c = value;
    } else {
        environment.temperature_valid = 0U;
    }

    if (ui.mode == CLOCK_UI_VIEW) {
        if (active_screen == APP_SCREEN_CLOCK) {
            clock_ui_update_main_cards(&environment, &previous_environment,
                                       &oximeter_data, &oximeter_data);
        } else if (active_screen == APP_SCREEN_CLIMATE) {
            clock_ui_update_climate(&environment, &previous_environment);
        }
    }
}

static void copy_oximeter_data(void)
{
    const oximeter5_data_t *data = oximeter5_get_data(&oximeter);
    if (data != 0) oximeter_data = *data;
}

static void service_oximeter(void)
{
    if (!oximeter_available) return;
    previous_oximeter_data = oximeter_data;
    if (oximeter5_service(&oximeter) == OXIMETER5_OK) {
        copy_oximeter_data();
        if ((ui.mode == CLOCK_UI_VIEW) && (active_screen == APP_SCREEN_CLOCK)) {
            clock_ui_update_main_cards(&environment, &environment,
                                       &oximeter_data, &previous_oximeter_data);
        }
    }
}

static void poll_oximeter_temperature(void)
{
    int16_t temperature;
    if (!oximeter_available) return;
    previous_oximeter_data = oximeter_data;
    if (oximeter5_read_temperature_deci_c(&oximeter, &temperature) == OXIMETER5_OK) {
        oximeter.data.temperature_valid = 1U;
        oximeter.data.temperature_deci_c = temperature;
    } else {
        oximeter.data.temperature_valid = 0U;
    }
    copy_oximeter_data();
}

int main(void)
{
    rtc24_cfg_t rtc_cfg;
    oximeter5_cfg_t ox_cfg;
    uint8_t rtc_ok = 0U;
    uint8_t previous_sqw = 0U;
    uint16_t blink_elapsed = 0U;
    uint16_t rtc_fallback_elapsed = 0U;
    uint16_t thermo_elapsed = 0U;
    uint16_t ox_temp_elapsed = 0U;
    uint16_t ox_ui_elapsed = 0U;

#ifdef PREINIT_SUPPORTED
    preinit();
#endif

    if (ips_display2_init() != 0) {
        ips_display2_backlight(0U);
        while (1) Delay_ms(1000U);
    }

    buttons_init(&buttons);

    rtc24_cfg_setup(&rtc_cfg);
    rtc_cfg.scl = (bm_gpio_pin_t){ BOARD_RTC_SCL_PORT, BOARD_RTC_SCL_MASK };
    rtc_cfg.sda = (bm_gpio_pin_t){ BOARD_RTC_SDA_PORT, BOARD_RTC_SDA_MASK };
    rtc_cfg.sqw = (bm_gpio_pin_t){ BOARD_RTC_SQW_PORT, BOARD_RTC_SQW_MASK };
    rtc_cfg.i2c_half_period_us = BOARD_RTC_I2C_HALF_PERIOD_US;
    rtc_cfg.i2c_address = RTC24_DEVICE_ADDRESS;

    if ((rtc24_init(&rtc, &rtc_cfg) == RTC24_OK) &&
        (rtc24_default_cfg(&rtc) == RTC24_OK) &&
        (rtc24_read_datetime(&rtc, &live_datetime) == RTC24_OK)) {
        rtc_ok = 1U;
        rtc_available = 1U;
    } else {
        live_datetime.time.hour = 0U;
        live_datetime.time.minute = 0U;
        live_datetime.time.second = 0U;
        live_datetime.date.month = 1U;
        live_datetime.date.day = 1U;
        live_datetime.date.year = 26U;
        live_datetime.date.day_of_week = RTC24_THURSDAY;
    }

    thermo21_cfg_setup(&thermo);
    environment.temperature_valid = 0U;
    environment.temperature_deci_c = 0;
    previous_environment = environment;
    if (thermo21_init(&thermo) == THERMO21_OK) {
        thermo_available = 1U;
        poll_thermo21();
    }

    /* Oximeter 5 is assigned to the otherwise-free mikroBUS 2. */
    oximeter5_cfg_setup(&ox_cfg);
    oximeter_data.sensor_valid = 0U;
    oximeter_data.spo2_valid = 0U;
    oximeter_data.heart_rate_valid = 0U;
    oximeter_data.temperature_valid = 0U;
    oximeter_data.finger_present = 0U;
    oximeter_data.spo2_percent = OXIMETER5_SPO2_ERROR;
    oximeter_data.heart_rate_bpm = OXIMETER5_HEART_RATE_ERROR;
    oximeter_data.ir_raw = 0U;
    oximeter_data.red_raw = 0U;
    oximeter_data.temperature_deci_c = 0;

    if ((oximeter5_init(&oximeter, &ox_cfg) == OXIMETER5_OK) &&
        (oximeter5_default_cfg(&oximeter) == OXIMETER5_OK)) {
        oximeter_available = 1U;
        copy_oximeter_data();
        poll_oximeter_temperature();
    }
    previous_oximeter_data = oximeter_data;
    rendered_oximeter_data = oximeter_data;

    previous_datetime = live_datetime;
    ui.mode = CLOCK_UI_VIEW;
    ui.field = CLOCK_FIELD_HOUR;
    ui.blink_on = 1U;
    ui.edit_value = live_datetime;
    active_screen = APP_SCREEN_CLOCK;

    clock_ui_show_main(&live_datetime, rtc_ok, &environment, &oximeter_data);
    previous_sqw = rtc_ok ? rtc24_get_sqw_pin(&rtc) : 0U;

    while (1) {
        uint8_t events = buttons_poll(&buttons);

        /* Oximeter service is non-blocking and consumes a sample only when INT is low. */
        service_oximeter();

        thermo_elapsed = (uint16_t)(thermo_elapsed + BOARD_APP_TICK_MS);
        if (thermo_elapsed >= BOARD_THERMO_POLL_MS) {
            thermo_elapsed = 0U;
            if (thermo_available) poll_thermo21();
        }

        ox_temp_elapsed = (uint16_t)(ox_temp_elapsed + BOARD_APP_TICK_MS);
        if (ox_temp_elapsed >= BOARD_OXIMETER_TEMP_POLL_MS) {
            ox_temp_elapsed = 0U;
            poll_oximeter_temperature();
        }

        ox_ui_elapsed = (uint16_t)(ox_ui_elapsed + BOARD_APP_TICK_MS);
        if (ox_ui_elapsed >= BOARD_OXIMETER_UI_REFRESH_MS) {
            ox_ui_elapsed = 0U;
            if ((ui.mode == CLOCK_UI_VIEW) && (active_screen == APP_SCREEN_OXIMETER)) {
                clock_ui_update_oximeter(&oximeter_data, &rendered_oximeter_data);
                rendered_oximeter_data = oximeter_data;
            }
        }

        if (ui.mode == CLOCK_UI_VIEW) {
            if (events & BUTTON_B6) {
                next_screen();
            } else if ((active_screen == APP_SCREEN_CLOCK) && (events & BUTTON_B1)) {
                enter_editor();
                blink_elapsed = 0U;
            }

            if (rtc_ok) {
                uint8_t sqw = rtc24_get_sqw_pin(&rtc);
                rtc_fallback_elapsed = (uint16_t)(rtc_fallback_elapsed + BOARD_APP_TICK_MS);

                if (((previous_sqw == 0U) && (sqw != 0U)) || (rtc_fallback_elapsed >= 1200U)) {
                    previous_datetime = live_datetime;
                    if (rtc24_read_datetime(&rtc, &live_datetime) == RTC24_OK) {
                        if (active_screen == APP_SCREEN_CLOCK) {
                            clock_ui_update_main(&live_datetime, &previous_datetime);
                        }
                    }
                    rtc_fallback_elapsed = 0U;
                }
                previous_sqw = sqw;
            }
        } else {
            process_editor_buttons(events);

            if (ui.mode == CLOCK_UI_NAVIGATE) {
                blink_elapsed = (uint16_t)(blink_elapsed + BOARD_APP_TICK_MS);
                if (blink_elapsed >= 500U) {
                    blink_elapsed = 0U;
                    ui.blink_on = ui.blink_on ? 0U : 1U;
                    clock_ui_redraw_field(&ui, ui.field);
                }
            } else {
                blink_elapsed = 0U;
            }
        }

        Delay_ms(BOARD_APP_TICK_MS);
    }

    return 0;
}
