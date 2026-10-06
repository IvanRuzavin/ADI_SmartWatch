#include "gfx.h"
#include "ips_display2.h"

#define FONT_W 5U
#define FONT_H 7U
#define MAX_SCALE 3U

static void glyph_rows(char c, uint8_t rows[FONT_H])
{
    uint8_t i;
    for (i = 0U; i < FONT_H; ++i) rows[i] = 0U;

    switch (c) {
        case '0': { const uint8_t r[7]={0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '1': { const uint8_t r[7]={0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '2': { const uint8_t r[7]={0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '3': { const uint8_t r[7]={0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '4': { const uint8_t r[7]={0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '5': { const uint8_t r[7]={0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '6': { const uint8_t r[7]={0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '7': { const uint8_t r[7]={0x1F,0x01,0x02,0x04,0x08,0x08,0x08}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '8': { const uint8_t r[7]={0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '9': { const uint8_t r[7]={0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'A': { const uint8_t r[7]={0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'B': { const uint8_t r[7]={0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'C': { const uint8_t r[7]={0x0F,0x10,0x10,0x10,0x10,0x10,0x0F}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'D': { const uint8_t r[7]={0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'E': { const uint8_t r[7]={0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'F': { const uint8_t r[7]={0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'G': { const uint8_t r[7]={0x0F,0x10,0x10,0x13,0x11,0x11,0x0F}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'H': { const uint8_t r[7]={0x11,0x11,0x11,0x1F,0x11,0x11,0x11}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'I': { const uint8_t r[7]={0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'J': { const uint8_t r[7]={0x07,0x02,0x02,0x02,0x12,0x12,0x0C}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'K': { const uint8_t r[7]={0x11,0x12,0x14,0x18,0x14,0x12,0x11}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'L': { const uint8_t r[7]={0x10,0x10,0x10,0x10,0x10,0x10,0x1F}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'M': { const uint8_t r[7]={0x11,0x1B,0x15,0x15,0x11,0x11,0x11}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'N': { const uint8_t r[7]={0x11,0x19,0x15,0x13,0x11,0x11,0x11}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'O': { const uint8_t r[7]={0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'P': { const uint8_t r[7]={0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'Q': { const uint8_t r[7]={0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'R': { const uint8_t r[7]={0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'S': { const uint8_t r[7]={0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'T': { const uint8_t r[7]={0x1F,0x04,0x04,0x04,0x04,0x04,0x04}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'U': { const uint8_t r[7]={0x11,0x11,0x11,0x11,0x11,0x11,0x0E}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'V': { const uint8_t r[7]={0x11,0x11,0x11,0x11,0x11,0x0A,0x04}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'W': { const uint8_t r[7]={0x11,0x11,0x11,0x15,0x15,0x15,0x0A}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'X': { const uint8_t r[7]={0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'Y': { const uint8_t r[7]={0x11,0x11,0x0A,0x04,0x04,0x04,0x04}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case 'Z': { const uint8_t r[7]={0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case ':': { const uint8_t r[7]={0x00,0x04,0x04,0x00,0x04,0x04,0x00}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '.': { const uint8_t r[7]={0x00,0x00,0x00,0x00,0x00,0x0C,0x0C}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '%': { const uint8_t r[7]={0x19,0x19,0x02,0x04,0x08,0x13,0x13}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '+': { const uint8_t r[7]={0x00,0x04,0x04,0x1F,0x04,0x04,0x00}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '-': { const uint8_t r[7]={0x00,0x00,0x00,0x1F,0x00,0x00,0x00}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case '/': { const uint8_t r[7]={0x01,0x02,0x02,0x04,0x08,0x08,0x10}; for(i=0;i<7;i++)rows[i]=r[i]; } break;
        case ' ': default: break;
    }
}

void gfx_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    (void)ips_display2_fill_rect(x, y, w, h, color);
}

void gfx_fill_chamfer_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                           uint16_t radius, uint16_t color)
{
    if ((w == 0U) || (h == 0U)) return;
    if ((radius == 0U) || (w <= (uint16_t)(2U * radius)) || (h <= (uint16_t)(2U * radius))) {
        gfx_fill_rect(x, y, w, h, color);
        return;
    }

    gfx_fill_rect((uint16_t)(x + radius), y, (uint16_t)(w - 2U * radius), h, color);
    gfx_fill_rect(x, (uint16_t)(y + radius), w, (uint16_t)(h - 2U * radius), color);
}

void gfx_draw_char(uint16_t x, uint16_t y, char c, uint8_t scale,
                   uint16_t fg, uint16_t bg)
{
    static uint16_t pixels[FONT_W * MAX_SCALE * FONT_H * MAX_SCALE];
    uint8_t rows[FONT_H];
    uint16_t width;
    uint16_t height;
    uint16_t py;
    uint16_t px;

    if ((scale == 0U) || (scale > MAX_SCALE)) return;

    glyph_rows(c, rows);
    width = (uint16_t)(FONT_W * scale);
    height = (uint16_t)(FONT_H * scale);

    for (py = 0U; py < height; ++py) {
        uint8_t row = rows[py / scale];
        for (px = 0U; px < width; ++px) {
            uint8_t bit = (uint8_t)(4U - (px / scale));
            pixels[py * width + px] = (row & (1U << bit)) ? fg : bg;
        }
    }

    (void)ips_display2_draw_rgb565_image(x, y, width, height, pixels);
}

uint16_t gfx_text_width(const char *text, uint8_t scale)
{
    uint16_t length = 0U;
    if ((text == 0) || (scale == 0U)) return 0U;
    while (*text++) length++;
    if (length == 0U) return 0U;
    return (uint16_t)(length * 6U * scale - scale);
}

void gfx_draw_text(uint16_t x, uint16_t y, const char *text, uint8_t scale,
                   uint16_t fg, uint16_t bg)
{
    if (text == 0) return;
    while (*text) {
        gfx_draw_char(x, y, *text++, scale, fg, bg);
        x = (uint16_t)(x + 6U * scale);
    }
}

void gfx_draw_text_centered(uint16_t x, uint16_t y, uint16_t w, const char *text,
                            uint8_t scale, uint16_t fg, uint16_t bg)
{
    uint16_t tw = gfx_text_width(text, scale);
    uint16_t tx = (tw < w) ? (uint16_t)(x + (w - tw) / 2U) : x;
    gfx_draw_text(tx, y, text, scale, fg, bg);
}

void gfx_draw_7seg_digit(uint16_t x, uint16_t y, uint8_t digit,
                         uint16_t fg, uint16_t bg)
{
    static const uint8_t masks[10] = {
        0x3FU, /* 0: A B C D E F */
        0x06U, /* 1: B C */
        0x5BU, /* 2: A B D E G */
        0x4FU, /* 3: A B C D G */
        0x66U, /* 4: B C F G */
        0x6DU, /* 5: A C D F G */
        0x7DU, /* 6: A C D E F G */
        0x07U, /* 7: A B C */
        0x7FU, /* 8 */
        0x6FU  /* 9 */
    };
    uint8_t m = (digit < 10U) ? masks[digit] : 0U;
    const uint16_t w = 22U;
    const uint16_t h = 42U;
    const uint16_t t = 4U;

    gfx_fill_rect(x, y, w, h, bg);
    if (m & 0x01U) gfx_fill_chamfer_rect((uint16_t)(x+t), y, (uint16_t)(w-2U*t), t, 1U, fg);           /* A */
    if (m & 0x02U) gfx_fill_chamfer_rect((uint16_t)(x+w-t), (uint16_t)(y+t), t, (uint16_t)(h/2U-t), 1U, fg); /* B */
    if (m & 0x04U) gfx_fill_chamfer_rect((uint16_t)(x+w-t), (uint16_t)(y+h/2U), t, (uint16_t)(h/2U-t), 1U, fg); /* C */
    if (m & 0x08U) gfx_fill_chamfer_rect((uint16_t)(x+t), (uint16_t)(y+h-t), (uint16_t)(w-2U*t), t, 1U, fg);    /* D */
    if (m & 0x10U) gfx_fill_chamfer_rect(x, (uint16_t)(y+h/2U), t, (uint16_t)(h/2U-t), 1U, fg);                /* E */
    if (m & 0x20U) gfx_fill_chamfer_rect(x, (uint16_t)(y+t), t, (uint16_t)(h/2U-t), 1U, fg);                  /* F */
    if (m & 0x40U) gfx_fill_chamfer_rect((uint16_t)(x+t), (uint16_t)(y+h/2U-t/2U), (uint16_t)(w-2U*t), t, 1U, fg); /* G */
}

void gfx_draw_7seg_colon(uint16_t x, uint16_t y, uint16_t fg, uint16_t bg)
{
    gfx_fill_rect(x, y, 8U, 42U, bg);
    gfx_fill_chamfer_rect((uint16_t)(x+2U), (uint16_t)(y+11U), 4U, 4U, 1U, fg);
    gfx_fill_chamfer_rect((uint16_t)(x+2U), (uint16_t)(y+27U), 4U, 4U, 1U, fg);
}
