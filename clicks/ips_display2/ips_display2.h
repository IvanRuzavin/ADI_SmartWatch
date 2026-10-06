#ifndef IPS_DISPLAY2_BARE_METAL_H
#define IPS_DISPLAY2_BARE_METAL_H

#include <stdint.h>

#define IPS_DISPLAY2_WIDTH       (240U)
#define IPS_DISPLAY2_HEIGHT      (240U)

#define IPS_COLOR_BLACK          (0x0000U)
#define IPS_COLOR_WHITE          (0xFFFFU)
#define IPS_COLOR_RED            (0xF800U)
#define IPS_COLOR_GREEN          (0x07E0U)
#define IPS_COLOR_BLUE           (0x001FU)
#define IPS_COLOR_YELLOW         (0xFFE0U)
#define IPS_COLOR_CYAN           (0x07FFU)
#define IPS_COLOR_MAGENTA        (0xF81FU)
#define IPS_COLOR_GRAY           (0x8410U)

int ips_display2_init(void);
int ips_display2_fill_screen(uint16_t color);
int ips_display2_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
int ips_display2_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
int ips_display2_draw_rgb565_image(uint16_t x, uint16_t y,
                                   uint16_t width, uint16_t height,
                                   const uint16_t *pixels);
void ips_display2_backlight(uint8_t enable);

#endif /* IPS_DISPLAY2_BARE_METAL_H */
