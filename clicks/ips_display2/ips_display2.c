#include "ips_display2.h"
#include "bm_gpio.h"
#include "bm_spi.h"
#include "board_config.h"
#include "delays.h"

/* ST7789V commands used by IPS Display 2 Click. */
#define CMD_SLPOUT      0x11U
#define CMD_INVON       0x21U
#define CMD_DISPON      0x29U
#define CMD_CASET       0x2AU
#define CMD_RASET       0x2BU
#define CMD_RAMWR       0x2CU
#define CMD_MADCTL      0x36U
#define CMD_COLMOD      0x3AU
#define CMD_PORCTRL     0xB2U
#define CMD_GCTRL       0xB7U
#define CMD_VCOMS       0xBBU
#define CMD_VDVVRHEN    0xC2U
#define CMD_VRHS        0xC3U
#define CMD_VDVSET      0xC4U
#define CMD_FRCTR2      0xC6U
#define CMD_PWCTRL1     0xD0U
#define CMD_PVGAMCTRL   0xE0U
#define CMD_NVGAMCTRL   0xE1U

/* Rotation matches the Click package's IPSDISPLAY2_ROTATION_VERTICAL_0. */
#define MADCTL_VERTICAL_0   0x00U

/*
 * Match the known-good MikroE IPS Display 2 package exactly for bring-up.
 * The package sends only the control-interface field: 16-bit/pixel = 0x05.
 * Do not "correct" this to 0x55 until the reference waveform is working.
 */
#define COLMOD_RGB565       0x05U

static const bm_gpio_pin_t pin_cs  = { BOARD_IPS_CS_PORT,  BOARD_IPS_CS_MASK };
static const bm_gpio_pin_t pin_rst = { BOARD_IPS_RST_PORT, BOARD_IPS_RST_MASK };
static const bm_gpio_pin_t pin_dc  = { BOARD_IPS_DC_PORT,  BOARD_IPS_DC_MASK };
static const bm_gpio_pin_t pin_bck = { BOARD_IPS_BCK_PORT, BOARD_IPS_BCK_MASK };

static int write_bytes(const uint8_t *data, uint32_t length)
{
    return (bm_spi1_write(data, length) == BM_SPI_OK) ? 0 : -1;
}

static int write_command(uint8_t command, const uint8_t *params, uint32_t param_count)
{
    uint32_t i;

    /*
     * IMPORTANT: mirror the known-good MikroE driver transaction boundaries.
     * The command is one CS transaction. Every parameter byte is then sent as
     * a separate CS transaction with D/C already high before CS goes low.
     */
    bm_gpio_clear(pin_dc);
    bm_gpio_clear(pin_cs);
    if (write_bytes(&command, 1U) != 0) {
        bm_gpio_set(pin_cs);
        return -1;
    }
    bm_gpio_set(pin_cs);

    for (i = 0U; (i < param_count) && (params != 0); ++i) {
        bm_gpio_set(pin_dc);
        bm_gpio_clear(pin_cs);
        if (write_bytes(&params[i], 1U) != 0) {
            bm_gpio_set(pin_cs);
            return -1;
        }
        bm_gpio_set(pin_cs);
    }

    return 0;
}

static void hardware_reset(void)
{
    bm_gpio_clear(pin_rst);
    Delay_ms(1);
    bm_gpio_set(pin_rst);
    Delay_ms(200);
}

static int set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t data[4];

    if ((x0 > x1) || (y0 > y1) || (x1 >= IPS_DISPLAY2_WIDTH) || (y1 >= IPS_DISPLAY2_HEIGHT)) {
        return -1;
    }

    data[0] = (uint8_t)(x0 >> 8);
    data[1] = (uint8_t)x0;
    data[2] = (uint8_t)(x1 >> 8);
    data[3] = (uint8_t)x1;
    if (write_command(CMD_CASET, data, 4U) != 0) {
        return -1;
    }

    data[0] = (uint8_t)(y0 >> 8);
    data[1] = (uint8_t)y0;
    data[2] = (uint8_t)(y1 >> 8);
    data[3] = (uint8_t)y1;
    if (write_command(CMD_RASET, data, 4U) != 0) {
        return -1;
    }

    return write_command(CMD_RAMWR, 0, 0U);
}

static int write_solid_pixels(uint16_t color, uint32_t pixel_count)
{
    uint8_t pixel[2];

    pixel[0] = (uint8_t)(color >> 8);
    pixel[1] = (uint8_t)color;

    /*
     * Intentionally slow reference path: the working MikroE implementation
     * asserts CS for exactly one RGB565 pixel (two bytes), then deasserts it.
     * This also limits every MAX32690 SPI hardware transaction to two bytes.
     */
    bm_gpio_set(pin_dc);
    while (pixel_count-- > 0U) {
        bm_gpio_clear(pin_cs);
        if (write_bytes(pixel, 2U) != 0) {
            bm_gpio_set(pin_cs);
            return -1;
        }
        bm_gpio_set(pin_cs);
    }

    return 0;
}

int ips_display2_init(void)
{
    static const uint8_t porctrl[] = { 0x0CU, 0x0CU, 0x00U, 0x33U, 0x33U };
    static const uint8_t pwctrl1[] = { 0xA4U, 0xA1U };
    static const uint8_t gamma_pos[] = {
        0xD0U, 0x08U, 0x0EU, 0x09U, 0x09U, 0x05U, 0x31U,
        0x33U, 0x48U, 0x17U, 0x14U, 0x15U, 0x31U, 0x34U
    };
    static const uint8_t gamma_neg[] = {
        0xD0U, 0x08U, 0x0EU, 0x09U, 0x09U, 0x15U, 0x31U,
        0x33U, 0x48U, 0x17U, 0x14U, 0x15U, 0x31U, 0x34U
    };
    uint8_t value;

    bm_gpio_init_output(pin_cs, 1U);
    bm_gpio_init_output(pin_rst, 1U);
    bm_gpio_init_output(pin_dc, 1U);
    bm_gpio_init_output(pin_bck, 0U);

    if (bm_spi1_init(BOARD_IPS_SPI_HZ) != BM_SPI_OK) {
        return -1;
    }

    hardware_reset();

    if (write_command(CMD_SLPOUT, 0, 0U) != 0) return -1;
    Delay_ms(120);

    value = MADCTL_VERTICAL_0;
    if (write_command(CMD_MADCTL, &value, 1U) != 0) return -1;

    value = COLMOD_RGB565;
    if (write_command(CMD_COLMOD, &value, 1U) != 0) return -1;

    if (write_command(CMD_PORCTRL, porctrl, sizeof(porctrl)) != 0) return -1;

    value = 0x35U;
    if (write_command(CMD_GCTRL, &value, 1U) != 0) return -1;

    value = 0x3AU;
    if (write_command(CMD_VCOMS, &value, 1U) != 0) return -1;

    value = 0x01U;
    if (write_command(CMD_VDVVRHEN, &value, 1U) != 0) return -1;

    value = 0x19U;
    if (write_command(CMD_VRHS, &value, 1U) != 0) return -1;

    value = 0x20U;
    if (write_command(CMD_VDVSET, &value, 1U) != 0) return -1;

    value = 0x0FU;
    if (write_command(CMD_FRCTR2, &value, 1U) != 0) return -1;

    if (write_command(CMD_PWCTRL1, pwctrl1, sizeof(pwctrl1)) != 0) return -1;
    if (write_command(CMD_PVGAMCTRL, gamma_pos, sizeof(gamma_pos)) != 0) return -1;
    if (write_command(CMD_NVGAMCTRL, gamma_neg, sizeof(gamma_neg)) != 0) return -1;
    if (write_command(CMD_INVON, 0, 0U) != 0) return -1;

    if (ips_display2_fill_screen(IPS_COLOR_BLACK) != 0) return -1;
    if (write_command(CMD_DISPON, 0, 0U) != 0) return -1;

    ips_display2_backlight(1U);
    Delay_ms(100);
    return 0;
}

void ips_display2_backlight(uint8_t enable)
{
    bm_gpio_write(pin_bck, enable ? 1U : 0U);
}

int ips_display2_fill_screen(uint16_t color)
{
    if (set_window(0U, 0U, IPS_DISPLAY2_WIDTH - 1U, IPS_DISPLAY2_HEIGHT - 1U) != 0) {
        return -1;
    }
    return write_solid_pixels(color, (uint32_t)IPS_DISPLAY2_WIDTH * IPS_DISPLAY2_HEIGHT);
}

int ips_display2_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    uint16_t x1;
    uint16_t y1;

    if ((width == 0U) || (height == 0U) || (x >= IPS_DISPLAY2_WIDTH) || (y >= IPS_DISPLAY2_HEIGHT)) {
        return -1;
    }

    x1 = (uint16_t)(x + width - 1U);
    y1 = (uint16_t)(y + height - 1U);
    if ((x1 >= IPS_DISPLAY2_WIDTH) || (y1 >= IPS_DISPLAY2_HEIGHT)) {
        return -1;
    }

    if (set_window(x, y, x1, y1) != 0) {
        return -1;
    }

    return write_solid_pixels(color, (uint32_t)width * height);
}

int ips_display2_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    return ips_display2_fill_rect(x, y, 1U, 1U, color);
}

int ips_display2_draw_rgb565_image(uint16_t x, uint16_t y,
                                   uint16_t width, uint16_t height,
                                   const uint16_t *pixels)
{
    uint32_t count;

    if ((pixels == 0) || (width == 0U) || (height == 0U) ||
        ((uint32_t)x + width > IPS_DISPLAY2_WIDTH) ||
        ((uint32_t)y + height > IPS_DISPLAY2_HEIGHT)) {
        return -1;
    }

    if (set_window(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U)) != 0) {
        return -1;
    }

    count = (uint32_t)width * height;
    bm_gpio_set(pin_dc);

    while (count-- > 0U) {
        uint16_t pixel_value = *pixels++;
        uint8_t pixel[2];
        pixel[0] = (uint8_t)(pixel_value >> 8);
        pixel[1] = (uint8_t)pixel_value;

        bm_gpio_clear(pin_cs);
        if (write_bytes(pixel, 2U) != 0) {
            bm_gpio_set(pin_cs);
            return -1;
        }
        bm_gpio_set(pin_cs);
    }

    return 0;
}
