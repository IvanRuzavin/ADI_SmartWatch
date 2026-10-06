#include "buttons.h"
#include "board_config.h"

static uint8_t raw_pressed(const button_state_t *button)
{
    uint8_t level = bm_gpio_read(button->pin);
#if BOARD_BUTTON_ACTIVE_LOW
    return level ? 0U : 1U;
#else
    return level ? 1U : 0U;
#endif
}

static void setup_button(button_state_t *button, bm_gpio_pin_t pin, uint8_t event_mask)
{
    button->pin = pin;
    button->stable_pressed = 0U;
    button->last_sample_pressed = 0U;
    button->debounce_count = 0U;
    button->event_mask = event_mask;
    button->hold_ticks = 0U;
    bm_gpio_init_input(pin, BOARD_BUTTON_ACTIVE_LOW ? 1U : 0U);
}

void buttons_init(buttons_t *ctx)
{
    if (ctx == 0) {
        return;
    }

    setup_button(&ctx->buttons[0], (bm_gpio_pin_t){ BOARD_B1_PORT, BOARD_B1_MASK }, BUTTON_B1);
    setup_button(&ctx->buttons[1], (bm_gpio_pin_t){ BOARD_B2_PORT, BOARD_B2_MASK }, BUTTON_B2);
    setup_button(&ctx->buttons[2], (bm_gpio_pin_t){ BOARD_B3_PORT, BOARD_B3_MASK }, BUTTON_B3);
    setup_button(&ctx->buttons[3], (bm_gpio_pin_t){ BOARD_B4_PORT, BOARD_B4_MASK }, BUTTON_B4);
    setup_button(&ctx->buttons[4], (bm_gpio_pin_t){ BOARD_B5_PORT, BOARD_B5_MASK }, BUTTON_B5);
    setup_button(&ctx->buttons[5], (bm_gpio_pin_t){ BOARD_B6_PORT, BOARD_B6_MASK }, BUTTON_B6);
}

uint8_t buttons_poll(buttons_t *ctx)
{
    uint8_t i;
    uint8_t events = 0U;

    if (ctx == 0) {
        return 0U;
    }

    for (i = 0U; i < 6U; ++i) {
        button_state_t *button = &ctx->buttons[i];
        uint8_t sample = raw_pressed(button);

        if (sample == button->last_sample_pressed) {
            if (button->debounce_count < BOARD_BUTTON_DEBOUNCE_TICKS) {
                button->debounce_count++;
            }
        } else {
            button->last_sample_pressed = sample;
            button->debounce_count = 1U;
        }

        if ((button->debounce_count >= BOARD_BUTTON_DEBOUNCE_TICKS) &&
            (button->stable_pressed != sample)) {
            button->stable_pressed = sample;
            button->hold_ticks = 0U;
            if (sample) {
                /* Every button generates one event immediately on a clean press. */
                events |= button->event_mask;
            }
        } else if (button->stable_pressed) {
            const uint16_t repeat_delay_ticks =
                (uint16_t)(BOARD_BUTTON_REPEAT_DELAY_MS / BOARD_APP_TICK_MS);
            const uint16_t repeat_interval_ticks =
                (uint16_t)(BOARD_BUTTON_REPEAT_INTERVAL_MS / BOARD_APP_TICK_MS);

            if (button->hold_ticks < 0xFFFFU) {
                button->hold_ticks++;
            }

            /* B4/B5 are spinner buttons: auto-repeat only after a 1 s hold. */
            if ((button->event_mask == BUTTON_B4) || (button->event_mask == BUTTON_B5)) {
                if (button->hold_ticks == repeat_delay_ticks) {
                    events |= button->event_mask;
                } else if ((button->hold_ticks > repeat_delay_ticks) &&
                           (repeat_interval_ticks != 0U) &&
                           (((button->hold_ticks - repeat_delay_ticks) % repeat_interval_ticks) == 0U)) {
                    events |= button->event_mask;
                }
            }
        }
    }

    return events;
}
