#ifndef CLOCK_UI_H
#define CLOCK_UI_H

#include <stdint.h>
#include "rtc24.h"
#include "oximeter5.h"

typedef enum {
    CLOCK_UI_VIEW = 0,
    CLOCK_UI_NAVIGATE,
    CLOCK_UI_EDIT_VALUE
} clock_ui_mode_t;

typedef enum {
    CLOCK_FIELD_HOUR = 0,
    CLOCK_FIELD_MINUTE,
    CLOCK_FIELD_SECOND,
    CLOCK_FIELD_MONTH,
    CLOCK_FIELD_DAY,
    CLOCK_FIELD_YEAR,
    CLOCK_FIELD_WEEKDAY,
    CLOCK_FIELD_COUNT
} clock_ui_field_t;

typedef enum {
    APP_SCREEN_CLOCK = 0,
    APP_SCREEN_CLIMATE,
    APP_SCREEN_OXIMETER,
    APP_SCREEN_COUNT
} app_screen_t;

typedef struct {
    uint8_t temperature_valid;
    int16_t temperature_deci_c;
} environment_data_t;

typedef struct {
    clock_ui_mode_t mode;
    clock_ui_field_t field;
    uint8_t blink_on;
    rtc24_datetime_t edit_value;
} clock_ui_state_t;

void clock_ui_show_main(const rtc24_datetime_t *dt, uint8_t rtc_ok,
                        const environment_data_t *env,
                        const oximeter5_data_t *oximeter);
void clock_ui_update_main(const rtc24_datetime_t *dt, const rtc24_datetime_t *previous);
void clock_ui_update_main_cards(const environment_data_t *env,
                                const environment_data_t *previous_env,
                                const oximeter5_data_t *oximeter,
                                const oximeter5_data_t *previous_oximeter);

void clock_ui_show_climate(const environment_data_t *env);
void clock_ui_update_climate(const environment_data_t *env,
                             const environment_data_t *previous);

void clock_ui_show_oximeter(const oximeter5_data_t *oximeter);
void clock_ui_update_oximeter(const oximeter5_data_t *oximeter,
                              const oximeter5_data_t *previous);

void clock_ui_show_editor(const clock_ui_state_t *state);
void clock_ui_redraw_field(const clock_ui_state_t *state, clock_ui_field_t field);

#endif /* CLOCK_UI_H */
