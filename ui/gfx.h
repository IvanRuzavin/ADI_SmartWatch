#ifndef CLOCK_GFX_H
#define CLOCK_GFX_H

#include <stdint.h>

#define GFX_RGB565(r, g, b) \
    ((uint16_t)((((uint16_t)(r) & 0xF8U) << 8) | \
                (((uint16_t)(g) & 0xFCU) << 3) | \
                (((uint16_t)(b) & 0xF8U) >> 3)))

void gfx_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void gfx_fill_chamfer_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                           uint16_t radius, uint16_t color);
void gfx_draw_char(uint16_t x, uint16_t y, char c, uint8_t scale,
                   uint16_t fg, uint16_t bg);
void gfx_draw_text(uint16_t x, uint16_t y, const char *text, uint8_t scale,
                   uint16_t fg, uint16_t bg);
void gfx_draw_text_centered(uint16_t x, uint16_t y, uint16_t w, const char *text,
                            uint8_t scale, uint16_t fg, uint16_t bg);
uint16_t gfx_text_width(const char *text, uint8_t scale);
void gfx_draw_7seg_digit(uint16_t x, uint16_t y, uint8_t digit,
                         uint16_t fg, uint16_t bg);
void gfx_draw_7seg_colon(uint16_t x, uint16_t y, uint16_t fg, uint16_t bg);

#endif /* CLOCK_GFX_H */
