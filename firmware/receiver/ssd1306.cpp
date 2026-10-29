#include <Arduino.h>
#include <Wire.h>
#include <avr/pgmspace.h>
#include "ssd1306.h"

#define W 128
#define H 64
#define PAGES (H / 8)

static uint8_t g_addr;
static bool    g_present;
static uint8_t g_buf[W];           /* the page currently being drawn */
static uint8_t g_page;

/* Classic 5x7 font, one entry per supported character: {char, 5 column bytes (bit0 = top)}.
 * Lower-case input is mapped to upper-case; unknown characters render as blank. */
struct Glyph { char c; uint8_t col[5]; };
static const Glyph kFont[] PROGMEM = {
    {' ', {0x00,0x00,0x00,0x00,0x00}}, {'!', {0x00,0x00,0x5F,0x00,0x00}},
    {'%', {0x23,0x13,0x08,0x64,0x62}}, {'(', {0x00,0x1C,0x22,0x41,0x00}},
    {')', {0x00,0x41,0x22,0x1C,0x00}}, {'+', {0x08,0x08,0x3E,0x08,0x08}},
    {',', {0x00,0x50,0x30,0x00,0x00}}, {'-', {0x08,0x08,0x08,0x08,0x08}},
    {'.', {0x00,0x60,0x60,0x00,0x00}}, {'/', {0x20,0x10,0x08,0x04,0x02}},
    {'0', {0x3E,0x51,0x49,0x45,0x3E}}, {'1', {0x00,0x42,0x7F,0x40,0x00}},
    {'2', {0x42,0x61,0x51,0x49,0x46}}, {'3', {0x21,0x41,0x45,0x4B,0x31}},
    {'4', {0x18,0x14,0x12,0x7F,0x10}}, {'5', {0x27,0x45,0x45,0x45,0x39}},
    {'6', {0x3C,0x4A,0x49,0x49,0x30}}, {'7', {0x01,0x71,0x09,0x05,0x03}},
    {'8', {0x36,0x49,0x49,0x49,0x36}}, {'9', {0x06,0x49,0x49,0x29,0x1E}},
    {':', {0x00,0x36,0x36,0x00,0x00}}, {'=', {0x14,0x14,0x14,0x14,0x14}},
    {'>', {0x00,0x41,0x22,0x14,0x08}}, {'?', {0x02,0x01,0x51,0x09,0x06}},
    {'A', {0x7E,0x11,0x11,0x11,0x7E}}, {'B', {0x7F,0x49,0x49,0x49,0x36}},
    {'C', {0x3E,0x41,0x41,0x41,0x22}}, {'D', {0x7F,0x41,0x41,0x22,0x1C}},
    {'E', {0x7F,0x49,0x49,0x49,0x41}}, {'F', {0x7F,0x09,0x09,0x09,0x01}},
    {'G', {0x3E,0x41,0x49,0x49,0x7A}}, {'H', {0x7F,0x08,0x08,0x08,0x7F}},
    {'I', {0x00,0x41,0x7F,0x41,0x00}}, {'J', {0x20,0x40,0x41,0x3F,0x01}},
    {'K', {0x7F,0x08,0x14,0x22,0x41}}, {'L', {0x7F,0x40,0x40,0x40,0x40}},
    {'M', {0x7F,0x02,0x0C,0x02,0x7F}}, {'N', {0x7F,0x04,0x08,0x10,0x7F}},
    {'O', {0x3E,0x41,0x41,0x41,0x3E}}, {'P', {0x7F,0x09,0x09,0x09,0x06}},
    {'Q', {0x3E,0x41,0x51,0x21,0x5E}}, {'R', {0x7F,0x09,0x19,0x29,0x46}},
    {'S', {0x46,0x49,0x49,0x49,0x31}}, {'T', {0x01,0x01,0x7F,0x01,0x01}},
    {'U', {0x3F,0x40,0x40,0x40,0x3F}}, {'V', {0x1F,0x20,0x40,0x20,0x1F}},
    {'W', {0x3F,0x40,0x38,0x40,0x3F}}, {'X', {0x63,0x14,0x08,0x14,0x63}},
    {'Y', {0x07,0x08,0x70,0x08,0x07}}, {'Z', {0x61,0x51,0x49,0x45,0x43}},
};
#define FONT_COUNT (sizeof(kFont) / sizeof(kFont[0]))

static void cmd(uint8_t c)
{
    Wire.beginTransmission(g_addr);
    Wire.write((uint8_t)0x00);
    Wire.write(c);
    Wire.endTransmission();
}

bool oled_begin(uint8_t i2c_addr)
{
    g_addr = i2c_addr;
    Wire.begin();
    Wire.setClock(400000);
    Wire.beginTransmission(g_addr);
    g_present = (Wire.endTransmission() == 0);
    if (!g_present)
        return false;
    static const uint8_t init[] PROGMEM = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00,
        0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    for (uint8_t i = 0; i < sizeof(init); i++)
        cmd(pgm_read_byte(&init[i]));
    return true;
}

bool oled_present(void) { return g_present; }

void oled_set_page(uint8_t page)
{
    g_page = page;
    for (uint8_t i = 0; i < W; i++)
        g_buf[i] = 0;
}

/* Page-relative pixel write; anything outside the current page is clipped. */
static void set_px(int16_t x, int16_t y, bool xor_mode)
{
    if (x < 0 || x >= W || y < 0 || y >= H)
        return;
    if ((uint8_t)(y >> 3) != g_page)
        return;
    uint8_t m = (uint8_t)(1u << (y & 7));
    if (xor_mode) g_buf[x] ^= m; else g_buf[x] |= m;
}

static void draw_char(int16_t x, int16_t y, char ch, uint8_t scale)
{
    if (y + 7 * scale <= (int16_t)g_page * 8 || y >= (int16_t)(g_page + 1) * 8)
        return;                                    /* glyph does not touch this page */
    if (ch >= 'a' && ch <= 'z')
        ch = (char)(ch - 32);
    uint8_t col[5] = {0, 0, 0, 0, 0};
    for (uint8_t i = 0; i < FONT_COUNT; i++) {
        if ((char)pgm_read_byte(&kFont[i].c) == ch) {
            for (uint8_t k = 0; k < 5; k++)
                col[k] = pgm_read_byte(&kFont[i].col[k]);
            break;
        }
    }
    for (uint8_t cx = 0; cx < 5; cx++)
        for (uint8_t ry = 0; ry < 7; ry++)
            if (col[cx] & (1u << ry))
                for (uint8_t sx = 0; sx < scale; sx++)
                    for (uint8_t sy = 0; sy < scale; sy++)
                        set_px(x + cx * scale + sx, y + ry * scale + sy, false);
}

int16_t oled_text_width(const char *s, uint8_t scale)
{
    int16_t n = 0;
    while (*s++) n++;
    return n ? (int16_t)(n * 6 * scale - scale) : 0;
}

void oled_text(int16_t x, int16_t y, const char *s, uint8_t scale)
{
    while (*s) {
        draw_char(x, y, *s++, scale);
        x += 6 * scale;
    }
}

void oled_hline(int16_t x, int16_t y, int16_t w)
{
    for (int16_t i = 0; i < w; i++)
        set_px(x + i, y, false);
}

void oled_invert_rect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    for (int16_t yy = y; yy < y + h; yy++)
        for (int16_t xx = x; xx < x + w; xx++)
            set_px(xx, yy, true);
}

void oled_flush_page(void)
{
    if (!g_present)
        return;
    cmd(0x21); cmd(0); cmd(W - 1);               /* column window */
    cmd(0x22); cmd(g_page); cmd(g_page);         /* page window */
    for (uint8_t chunk = 0; chunk < W / 16; chunk++) {
        Wire.beginTransmission(g_addr);
        Wire.write((uint8_t)0x40);
        Wire.write(&g_buf[chunk * 16], (size_t)16);
        Wire.endTransmission();
    }
}

void oled_power(bool on)
{
    if (g_present)
        cmd(on ? 0xAF : 0xAE);
}
