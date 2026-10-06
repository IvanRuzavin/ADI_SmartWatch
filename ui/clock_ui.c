#include "clock_ui.h"
#include "gfx.h"
#include "ips_display2.h"

#define C_BG          GFX_RGB565(8, 16, 30)
#define C_CARD        GFX_RGB565(18, 34, 56)
#define C_CARD_2      GFX_RGB565(22, 43, 68)
#define C_TEXT        GFX_RGB565(242, 247, 252)
#define C_MUTED       GFX_RGB565(132, 158, 184)
#define C_ACCENT      GFX_RGB565(45, 212, 230)
#define C_GREEN       GFX_RGB565(65, 209, 122)
#define C_YELLOW      GFX_RGB565(247, 190, 65)
#define C_ORANGE      GFX_RGB565(247, 132, 65)
#define C_RED         GFX_RGB565(235, 71, 111)
#define C_RED_DIM     GFX_RGB565(93, 33, 52)
#define C_BLUE        GFX_RGB565(59, 130, 246)
#define C_DISABLED    GFX_RGB565(65, 80, 99)
#define C_PURPLE      GFX_RGB565(142, 96, 246)

#define TIME_Y        73U
#define THERMO_GRAPH_MIN_DECI_C  (250)
#define THERMO_GRAPH_MAX_DECI_C  (430)

static const uint16_t time_digit_x[6] = { 26U, 52U, 90U, 116U, 154U, 180U };

static void two_digits(char out[3], uint8_t value)
{
    out[0] = (char)('0' + ((value / 10U) % 10U));
    out[1] = (char)('0' + (value % 10U));
    out[2] = '\0';
}

static void four_year(char out[5], uint8_t year)
{
    out[0] = '2'; out[1] = '0';
    out[2] = (char)('0' + ((year / 10U) % 10U));
    out[3] = (char)('0' + (year % 10U));
    out[4] = '\0';
}

static void date_string(char out[11], const rtc24_datetime_t *dt)
{
    out[0] = (char)('0' + dt->date.month / 10U);
    out[1] = (char)('0' + dt->date.month % 10U);
    out[2] = '.';
    out[3] = (char)('0' + dt->date.day / 10U);
    out[4] = (char)('0' + dt->date.day % 10U);
    out[5] = '.';
    out[6] = '2';
    out[7] = '0';
    out[8] = (char)('0' + dt->date.year / 10U);
    out[9] = (char)('0' + dt->date.year % 10U);
    out[10] = '\0';
}

static uint8_t display_hour_12(uint8_t hour24)
{
    uint8_t h = (uint8_t)(hour24 % 12U);
    return (h == 0U) ? 12U : h;
}

static uint8_t is_pm(uint8_t hour24)
{
    return (hour24 >= 12U) ? 1U : 0U;
}

static void rendered_time_digits(const rtc24_datetime_t *dt, uint8_t out[6])
{
    uint8_t h = display_hour_12(dt->time.hour);
    out[0] = h / 10U;
    out[1] = h % 10U;
    out[2] = dt->time.minute / 10U;
    out[3] = dt->time.minute % 10U;
    out[4] = dt->time.second / 10U;
    out[5] = dt->time.second % 10U;
}

static void draw_chip(uint16_t x, uint16_t y, uint16_t w, const char *text,
                      uint16_t fill, uint16_t fg)
{
    gfx_fill_chamfer_rect(x, y, w, 17U, 3U, fill);
    gfx_draw_text_centered(x, (uint16_t)(y + 5U), w, text, 1U, fg, fill);
}

static void draw_view_footer(void)
{
    gfx_fill_rect(0U, 190U, 240U, 50U, C_BG);
    draw_chip(8U, 193U, 72U, "B1 SET", C_ACCENT, C_TEXT);
    draw_chip(85U, 193U, 66U, "B2 --", C_DISABLED, C_MUTED);
    draw_chip(156U, 193U, 76U, "B3 --", C_DISABLED, C_MUTED);
    draw_chip(8U, 216U, 66U, "B4 --", C_DISABLED, C_MUTED);
    draw_chip(79U, 216U, 66U, "B5 --", C_DISABLED, C_MUTED);
    draw_chip(150U, 216U, 82U, "B6 NEXT", C_BLUE, C_TEXT);
}

static void draw_next_footer(void)
{
    gfx_fill_rect(0U, 210U, 240U, 30U, C_BG);
    draw_chip(75U, 216U, 90U, "B6 NEXT", C_BLUE, C_TEXT);
}

static void draw_editor_footer(clock_ui_mode_t mode)
{
    uint16_t b1 = C_GREEN;
    uint16_t b2 = (mode == CLOCK_UI_EDIT_VALUE) ? C_RED_DIM : C_CARD_2;
    uint16_t b3 = (mode == CLOCK_UI_NAVIGATE) ? C_BLUE : C_CARD_2;
    uint16_t b45 = (mode == CLOCK_UI_EDIT_VALUE) ? C_BLUE : C_DISABLED;

    gfx_fill_rect(0U, 187U, 240U, 53U, C_BG);
    draw_chip(8U, 190U, 72U, "B1 SAVE", b1, C_TEXT);
    draw_chip(85U, 190U, 90U, (mode == CLOCK_UI_EDIT_VALUE) ? "B2 BACK" : "B2 NEXT", b2, C_TEXT);
    draw_chip(180U, 190U, 52U, "B3 EDIT", b3, C_TEXT);
    draw_chip(26U, 213U, 44U, "B4 +", b45, C_TEXT);
    draw_chip(76U, 213U, 44U, "B5 -", b45, C_TEXT);
    draw_chip(126U, 213U, 82U, "B6 --", C_DISABLED, C_MUTED);
}

static void format_temp(char out[12], const environment_data_t *env)
{
    int16_t v;
    uint16_t mag;
    uint8_t p = 0U;

    if ((env == 0) || !env->temperature_valid) {
        out[0]='-'; out[1]='-'; out[2]=' '; out[3]='C'; out[4]='\0';
        return;
    }

    v = env->temperature_deci_c;
    if (v < 0) {
        out[p++]='-';
        mag=(uint16_t)(-v);
    } else {
        mag=(uint16_t)v;
    }

    if (mag >= 1000U) out[p++] = (char)('0' + ((mag / 1000U) % 10U));
    if (mag >= 100U) out[p++] = (char)('0' + ((mag / 100U) % 10U));
    else if (mag >= 1000U) out[p++]='0';
    out[p++] = (char)('0' + ((mag / 10U) % 10U));
    out[p++] = '.';
    out[p++] = (char)('0' + (mag % 10U));
    out[p++] = 'C';
    out[p] = '\0';
}

static void u32_string(char out[12], uint32_t value)
{
    char rev[10];
    uint8_t n = 0U;
    uint8_t i;
    if (value == 0U) {
        out[0] = '0'; out[1] = '\0'; return;
    }
    while ((value != 0U) && (n < sizeof(rev))) {
        rev[n++] = (char)('0' + (value % 10U));
        value /= 10U;
    }
    for (i = 0U; i < n; ++i) out[i] = rev[n - 1U - i];
    out[n] = '\0';
}

static void i32_string(char out[12], int32_t value)
{
    uint8_t p = 0U;
    uint32_t mag;
    char tmp[12];
    uint8_t i = 0U;
    if (value < 0) {
        out[p++] = '-';
        mag = (uint32_t)(-value);
    } else {
        mag = (uint32_t)value;
    }
    u32_string(tmp, mag);
    while (tmp[i] != '\0') out[p++] = tmp[i++];
    out[p] = '\0';
}

static void format_ox_temp(char out[12], const oximeter5_data_t *ox)
{
    environment_data_t env;
    env.temperature_valid = (ox != 0) ? ox->temperature_valid : 0U;
    env.temperature_deci_c = (ox != 0) ? ox->temperature_deci_c : 0;
    format_temp(out, &env);
}

static void draw_temperature_card(const environment_data_t *env)
{
    char value[12];
    format_temp(value, env);
    gfx_fill_chamfer_rect(10U, 137U, 105U, 43U, 5U, C_CARD_2);
    gfx_draw_text(18U, 143U, "TEMP", 1U, C_MUTED, C_CARD_2);
    gfx_draw_text_centered(12U, 158U, 101U, value, 2U,
                           (env && env->temperature_valid) ? C_ACCENT : C_DISABLED, C_CARD_2);
}

static void draw_spo2_card(const oximeter5_data_t *ox)
{
    char value[6];
    if ((ox != 0) && ox->spo2_valid) {
        uint8_t v = ox->spo2_percent;
        uint8_t p = 0U;
        if (v >= 100U) value[p++]='1';
        if (v >= 10U) value[p++] = (char)('0' + ((v / 10U) % 10U));
        value[p++] = (char)('0' + (v % 10U));
        value[p++] = '%';
        value[p] = '\0';
    } else {
        value[0]='-'; value[1]='-'; value[2]='%'; value[3]='\0';
    }

    gfx_fill_chamfer_rect(125U, 137U, 105U, 43U, 5U, C_CARD_2);
    gfx_draw_text(133U, 143U, "SPO2", 1U, C_MUTED, C_CARD_2);
    gfx_draw_text_centered(127U, 158U, 101U, value, 2U,
                           (ox && ox->spo2_valid) ? C_PURPLE : C_DISABLED, C_CARD_2);
}

static void draw_main_time_full(const rtc24_datetime_t *dt)
{
    uint8_t vals[6];
    uint8_t i;

    rendered_time_digits(dt, vals);
    gfx_fill_chamfer_rect(10U, 48U, 220U, 78U, 6U, C_CARD);

    for (i = 0U; i < 6U; ++i) {
        gfx_draw_7seg_digit(time_digit_x[i], TIME_Y, vals[i], C_TEXT, C_CARD);
    }
    gfx_draw_7seg_colon(77U, TIME_Y, C_ACCENT, C_CARD);
    gfx_draw_7seg_colon(141U, TIME_Y, C_ACCENT, C_CARD);
    draw_chip(172U, 53U, 46U, is_pm(dt->time.hour) ? "PM" : "AM", C_BLUE, C_TEXT);
}

void clock_ui_show_main(const rtc24_datetime_t *dt, uint8_t rtc_ok,
                        const environment_data_t *env,
                        const oximeter5_data_t *oximeter)
{
    char date[11];
    (void)rtc_ok;
    if (dt == 0) return;

    date_string(date, dt);
    (void)ips_display2_fill_screen(C_BG);
    draw_chip(10U, 10U, 48U, rtc24_day_name_short(dt->date.day_of_week), C_BLUE, C_TEXT);
    gfx_draw_text(70U, 15U, date, 2U, C_TEXT, C_BG);
    draw_main_time_full(dt);
    draw_temperature_card(env);
    draw_spo2_card(oximeter);
    draw_view_footer();
}

void clock_ui_update_main(const rtc24_datetime_t *dt, const rtc24_datetime_t *previous)
{
    uint8_t now_digits[6];
    uint8_t old_digits[6];
    uint8_t i;
    char date[11];

    if (dt == 0) return;
    if (previous == 0) {
        draw_main_time_full(dt);
        return;
    }

    rendered_time_digits(dt, now_digits);
    rendered_time_digits(previous, old_digits);
    for (i = 0U; i < 6U; ++i) {
        if (now_digits[i] != old_digits[i]) {
            gfx_draw_7seg_digit(time_digit_x[i], TIME_Y, now_digits[i], C_TEXT, C_CARD);
        }
    }

    if (is_pm(dt->time.hour) != is_pm(previous->time.hour)) {
        draw_chip(172U, 53U, 46U, is_pm(dt->time.hour) ? "PM" : "AM", C_BLUE, C_TEXT);
    }

    if ((dt->date.day != previous->date.day) ||
        (dt->date.month != previous->date.month) ||
        (dt->date.year != previous->date.year) ||
        (dt->date.day_of_week != previous->date.day_of_week)) {
        date_string(date, dt);
        gfx_fill_rect(0U, 0U, 240U, 42U, C_BG);
        draw_chip(10U, 10U, 48U, rtc24_day_name_short(dt->date.day_of_week), C_BLUE, C_TEXT);
        gfx_draw_text(70U, 15U, date, 2U, C_TEXT, C_BG);
    }
}

void clock_ui_update_main_cards(const environment_data_t *env,
                                const environment_data_t *previous_env,
                                const oximeter5_data_t *oximeter,
                                const oximeter5_data_t *previous_oximeter)
{
    if ((env != 0) && ((previous_env == 0) ||
        (env->temperature_valid != previous_env->temperature_valid) ||
        (env->temperature_deci_c != previous_env->temperature_deci_c))) {
        draw_temperature_card(env);
    }

    if ((oximeter != 0) && ((previous_oximeter == 0) ||
        (oximeter->spo2_valid != previous_oximeter->spo2_valid) ||
        (oximeter->spo2_percent != previous_oximeter->spo2_percent))) {
        draw_spo2_card(oximeter);
    }
}

static uint16_t thermometer_color(const environment_data_t *env)
{
    int16_t t;
    if ((env == 0) || !env->temperature_valid) return C_DISABLED;
    t = env->temperature_deci_c;
    if (t < 290) return C_BLUE;
    if (t < 330) return C_ACCENT;
    if (t < 370) return C_GREEN;
    if (t < 400) return C_ORANGE;
    return C_RED;
}

static void draw_thermometer(const environment_data_t *env)
{
    uint16_t color = thermometer_color(env);
    int32_t t = (env && env->temperature_valid) ? env->temperature_deci_c : THERMO_GRAPH_MIN_DECI_C;
    uint16_t fill_h = 0U;
    uint16_t y;

    /* Only the graphic is clamped to 25..43 C. The numeric value is never clamped. */
    if (t < THERMO_GRAPH_MIN_DECI_C) t = THERMO_GRAPH_MIN_DECI_C;
    if (t > THERMO_GRAPH_MAX_DECI_C) t = THERMO_GRAPH_MAX_DECI_C;
    if (env && env->temperature_valid) {
        fill_h = (uint16_t)(((t - THERMO_GRAPH_MIN_DECI_C) * 104L) /
                            (THERMO_GRAPH_MAX_DECI_C - THERMO_GRAPH_MIN_DECI_C));
    }

    gfx_fill_rect(58U, 42U, 124U, 144U, C_BG);
    gfx_draw_text(47U, 43U, "43", 1U, C_MUTED, C_BG);
    gfx_draw_text(47U, 93U, "34", 1U, C_MUTED, C_BG);
    gfx_draw_text(47U, 143U, "25", 1U, C_MUTED, C_BG);
    gfx_fill_rect(69U, 49U, 8U, 1U, C_DISABLED);
    gfx_fill_rect(69U, 99U, 8U, 1U, C_DISABLED);
    gfx_fill_rect(69U, 149U, 8U, 1U, C_DISABLED);

    gfx_fill_chamfer_rect(94U, 42U, 48U, 126U, 12U, C_CARD_2);
    gfx_fill_chamfer_rect(108U, 52U, 20U, 105U, 6U, C_BG);
    gfx_fill_chamfer_rect(86U, 151U, 64U, 50U, 16U, C_CARD_2);
    gfx_fill_chamfer_rect(97U, 162U, 42U, 29U, 10U, color);

    if (env && env->temperature_valid && fill_h) {
        y = (uint16_t)(156U - fill_h);
        gfx_fill_chamfer_rect(112U, y, 12U, fill_h, 4U, color);
    }
}

static void draw_climate_value(const environment_data_t *env)
{
    char temp[12];
    format_temp(temp, env);
    gfx_fill_rect(145U, 77U, 87U, 75U, C_BG);
    gfx_draw_text(151U, 81U, "THERMO 21", 1U, C_MUTED, C_BG);
    gfx_draw_text_centered(145U, 105U, 87U, temp, 2U,
                           (env && env->temperature_valid) ? thermometer_color(env) : C_DISABLED, C_BG);
    gfx_draw_text(151U, 133U, "GRAPH 25-43C", 1U, C_MUTED, C_BG);
}

void clock_ui_show_climate(const environment_data_t *env)
{
    (void)ips_display2_fill_screen(C_BG);
    gfx_draw_text_centered(0U, 12U, 240U, "TEMPERATURE", 2U, C_TEXT, C_BG);
    draw_thermometer(env);
    draw_climate_value(env);
    draw_next_footer();
}

void clock_ui_update_climate(const environment_data_t *env,
                             const environment_data_t *previous)
{
    if (env == 0) return;
    if ((previous == 0) ||
        (env->temperature_valid != previous->temperature_valid) ||
        (env->temperature_deci_c != previous->temperature_deci_c)) {
        draw_thermometer(env);
        draw_climate_value(env);
    }
}

static void draw_ox_spo2(const oximeter5_data_t *ox)
{
    char value[8];
    if (ox && ox->spo2_valid) {
        u32_string(value, ox->spo2_percent);
    } else {
        value[0]='-'; value[1]='-'; value[2]='\0';
    }
    gfx_fill_chamfer_rect(10U, 42U, 105U, 66U, 6U, C_CARD_2);
    gfx_draw_text(18U, 50U, "SPO2", 1U, C_MUTED, C_CARD_2);
    gfx_draw_text_centered(10U, 69U, 85U, value, 3U,
                           (ox && ox->spo2_valid) ? C_PURPLE : C_DISABLED, C_CARD_2);
    gfx_draw_text(91U, 81U, "%", 1U, C_MUTED, C_CARD_2);
}

static void draw_ox_hr(const oximeter5_data_t *ox)
{
    char value[12];
    if (ox && ox->heart_rate_valid) i32_string(value, ox->heart_rate_bpm);
    else { value[0]='-'; value[1]='-'; value[2]='\0'; }
    gfx_fill_chamfer_rect(125U, 42U, 105U, 66U, 6U, C_CARD_2);
    gfx_draw_text(133U, 50U, "HEART RATE", 1U, C_MUTED, C_CARD_2);
    gfx_draw_text_centered(125U, 69U, 105U, value, 3U,
                           (ox && ox->heart_rate_valid) ? C_RED : C_DISABLED, C_CARD_2);
    gfx_draw_text_centered(125U, 94U, 105U, "BPM", 1U, C_MUTED, C_CARD_2);
}

static void draw_raw_card(uint16_t x, uint16_t y, const char *label,
                          uint32_t value, uint16_t color)
{
    char text[12];
    u32_string(text, value);
    gfx_fill_chamfer_rect(x, y, 105U, 38U, 5U, C_CARD);
    gfx_draw_text((uint16_t)(x + 8U), (uint16_t)(y + 6U), label, 1U, C_MUTED, C_CARD);
    gfx_draw_text_centered(x, (uint16_t)(y + 20U), 105U, text, 1U, color, C_CARD);
}

static void draw_ox_temperature(const oximeter5_data_t *ox)
{
    char text[12];
    format_ox_temp(text, ox);
    gfx_fill_chamfer_rect(10U, 164U, 105U, 38U, 5U, C_CARD);
    gfx_draw_text(18U, 170U, "SENSOR TEMP", 1U, C_MUTED, C_CARD);
    gfx_draw_text_centered(10U, 184U, 105U, text, 1U,
                           (ox && ox->temperature_valid) ? C_ACCENT : C_DISABLED, C_CARD);
}

static void draw_ox_finger(const oximeter5_data_t *ox)
{
    const char *text;
    uint16_t color;
    if ((ox == 0) || !ox->sensor_valid) { text = "NO SENSOR"; color = C_DISABLED; }
    else if (ox->finger_present) { text = "FINGER ON"; color = C_GREEN; }
    else { text = "PLACE FINGER"; color = C_YELLOW; }
    gfx_fill_chamfer_rect(125U, 164U, 105U, 38U, 5U, C_CARD);
    gfx_draw_text(133U, 170U, "STATUS", 1U, C_MUTED, C_CARD);
    gfx_draw_text_centered(125U, 184U, 105U, text, 1U, color, C_CARD);
}

void clock_ui_show_oximeter(const oximeter5_data_t *oximeter)
{
    (void)ips_display2_fill_screen(C_BG);
    gfx_draw_text_centered(0U, 12U, 240U, "OXIMETER 5", 2U, C_TEXT, C_BG);
    draw_ox_spo2(oximeter);
    draw_ox_hr(oximeter);
    draw_raw_card(10U, 117U, "IR ADC", oximeter ? oximeter->ir_raw : 0U, C_ACCENT);
    draw_raw_card(125U, 117U, "RED ADC", oximeter ? oximeter->red_raw : 0U, C_RED);
    draw_ox_temperature(oximeter);
    draw_ox_finger(oximeter);
    draw_next_footer();
}

void clock_ui_update_oximeter(const oximeter5_data_t *oximeter,
                              const oximeter5_data_t *previous)
{
    if (oximeter == 0) return;
    if ((previous == 0) ||
        (oximeter->spo2_valid != previous->spo2_valid) ||
        (oximeter->spo2_percent != previous->spo2_percent)) draw_ox_spo2(oximeter);
    if ((previous == 0) ||
        (oximeter->heart_rate_valid != previous->heart_rate_valid) ||
        (oximeter->heart_rate_bpm != previous->heart_rate_bpm)) draw_ox_hr(oximeter);
    if ((previous == 0) || (oximeter->ir_raw != previous->ir_raw))
        draw_raw_card(10U, 117U, "IR ADC", oximeter->ir_raw, C_ACCENT);
    if ((previous == 0) || (oximeter->red_raw != previous->red_raw))
        draw_raw_card(125U, 117U, "RED ADC", oximeter->red_raw, C_RED);
    if ((previous == 0) ||
        (oximeter->temperature_valid != previous->temperature_valid) ||
        (oximeter->temperature_deci_c != previous->temperature_deci_c)) draw_ox_temperature(oximeter);
    if ((previous == 0) ||
        (oximeter->sensor_valid != previous->sensor_valid) ||
        (oximeter->finger_present != previous->finger_present)) draw_ox_finger(oximeter);
}

static void field_geometry(clock_ui_field_t field,
                           uint16_t *x, uint16_t *y, uint16_t *w, uint16_t *h)
{
    switch (field) {
        case CLOCK_FIELD_HOUR:    *x=8U;   *y=34U;  *w=68U;  *h=52U; break;
        case CLOCK_FIELD_MINUTE:  *x=86U;  *y=34U;  *w=68U;  *h=52U; break;
        case CLOCK_FIELD_SECOND:  *x=164U; *y=34U;  *w=68U;  *h=52U; break;
        case CLOCK_FIELD_MONTH:   *x=8U;   *y=94U;  *w=54U;  *h=52U; break;
        case CLOCK_FIELD_DAY:     *x=68U;  *y=94U;  *w=54U;  *h=52U; break;
        case CLOCK_FIELD_YEAR:    *x=128U; *y=94U;  *w=104U; *h=52U; break;
        case CLOCK_FIELD_WEEKDAY: *x=8U;   *y=154U; *w=224U; *h=28U; break;
        default:                  *x=0U;   *y=0U;   *w=0U;   *h=0U; break;
    }
}

static const char *field_label(clock_ui_field_t field)
{
    static const char *labels[CLOCK_FIELD_COUNT] = {
        "HH", "MM", "SS", "MONTH", "DAY", "YEAR", "WEEKDAY"
    };
    return (field < CLOCK_FIELD_COUNT) ? labels[field] : "";
}

void clock_ui_redraw_field(const clock_ui_state_t *state, clock_ui_field_t field)
{
    uint16_t x,y,w,h;
    uint16_t bg = C_CARD;
    char value[5];

    if ((state == 0) || (field >= CLOCK_FIELD_COUNT)) return;
    field_geometry(field, &x, &y, &w, &h);

    if (field == state->field) {
        if (state->mode == CLOCK_UI_EDIT_VALUE) bg = C_BLUE;
        else if (state->mode == CLOCK_UI_NAVIGATE) bg = state->blink_on ? C_RED : C_RED_DIM;
    }

    gfx_fill_chamfer_rect(x, y, w, h, 4U, bg);
    if (field != CLOCK_FIELD_WEEKDAY) {
        gfx_draw_text_centered(x, (uint16_t)(y + 5U), w, field_label(field), 1U, C_MUTED, bg);
    }

    switch (field) {
        case CLOCK_FIELD_HOUR: {
            uint8_t h12 = display_hour_12(state->edit_value.time.hour);
            two_digits(value, h12);
            gfx_draw_text_centered(x, (uint16_t)(y + 22U), w, value, 3U, C_TEXT, bg);
            gfx_draw_text((uint16_t)(x + 43U), (uint16_t)(y + 6U),
                          is_pm(state->edit_value.time.hour) ? "PM" : "AM", 1U, C_MUTED, bg);
            break;
        }
        case CLOCK_FIELD_MINUTE:
            two_digits(value, state->edit_value.time.minute);
            gfx_draw_text_centered(x, (uint16_t)(y + 22U), w, value, 3U, C_TEXT, bg); break;
        case CLOCK_FIELD_SECOND:
            two_digits(value, state->edit_value.time.second);
            gfx_draw_text_centered(x, (uint16_t)(y + 22U), w, value, 3U, C_TEXT, bg); break;
        case CLOCK_FIELD_MONTH:
            two_digits(value, state->edit_value.date.month);
            gfx_draw_text_centered(x, (uint16_t)(y + 24U), w, value, 2U, C_TEXT, bg); break;
        case CLOCK_FIELD_DAY:
            two_digits(value, state->edit_value.date.day);
            gfx_draw_text_centered(x, (uint16_t)(y + 24U), w, value, 2U, C_TEXT, bg); break;
        case CLOCK_FIELD_YEAR:
            four_year(value, state->edit_value.date.year);
            gfx_draw_text_centered(x, (uint16_t)(y + 24U), w, value, 2U, C_TEXT, bg); break;
        case CLOCK_FIELD_WEEKDAY:
            gfx_draw_text(x + 10U, y + 10U, "WEEKDAY", 1U, C_MUTED, bg);
            gfx_draw_text_centered((uint16_t)(x + 112U), (uint16_t)(y + 8U), 100U,
                                   rtc24_day_name_short(state->edit_value.date.day_of_week),
                                   2U, C_TEXT, bg); break;
        default: break;
    }
}

void clock_ui_show_editor(const clock_ui_state_t *state)
{
    clock_ui_field_t field;
    if (state == 0) return;

    (void)ips_display2_fill_screen(C_BG);
    gfx_draw_text(10U, 10U, "SET CLOCK", 2U, C_TEXT, C_BG);
    gfx_draw_text(154U, 15U, "RED NAV", 1U, C_RED, C_BG);

    for (field = CLOCK_FIELD_HOUR; field < CLOCK_FIELD_COUNT; ++field) {
        clock_ui_redraw_field(state, field);
    }
    draw_editor_footer(state->mode);
}
