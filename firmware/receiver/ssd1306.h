/*
 * ssd1306.h - minimal SSD1306 128x64 I2C driver, page-at-a-time rendering.
 *
 * The ATmega328P has only 2 KB RAM, so there is no 1 KB framebuffer. Instead the UI draws the
 * whole screen once per 8-pixel page into a 128-byte buffer (drawing calls clip to the current
 * page) and sends that page; a full refresh is 8 passes. Each pass takes a few milliseconds, so
 * a refresh never blocks radio reception for long.
 */
#ifndef SSD1306_H
#define SSD1306_H
#include <stdint.h>
#include <stdbool.h>

bool oled_begin(uint8_t i2c_addr);     /* false if the display does not ACK on I2C */
bool oled_present(void);

/* Select page 0..7 and clear its buffer; subsequent draw calls are clipped to this page. */
void oled_set_page(uint8_t page);
void oled_text(int16_t x, int16_t y, const char *s, uint8_t scale);
int16_t oled_text_width(const char *s, uint8_t scale);
void oled_hline(int16_t x, int16_t y, int16_t w);
void oled_invert_rect(int16_t x, int16_t y, int16_t w, int16_t h);
/* Transfer the current page buffer to the display. */
void oled_flush_page(void);
void oled_power(bool on);
#endif
