#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include "ssd1306.h"
#include "oled_ui.h"

static UiState g_shown;
static bool    g_have_shown;

static void two_digits(char *out, uint8_t v)
{
    out[0] = (char)('0' + v / 10);
    out[1] = (char)('0' + v % 10);
    out[2] = 0;
}

static void draw(const UiState &s)
{
    char buf[24];

    /* header: title left, status right */
    oled_text(0, 0, "CALLING BELL", 1);
    const char *st = s.radio_ok ? "RF OK" : "RF FAIL";
    int16_t x = (int16_t)(128 - oled_text_width(st, 1));
    oled_text(x, 0, st, 1);
    oled_hline(0, 9, 128);

    if (s.count == 0) {
        const char *a = "NO CALLS";
        oled_text((int16_t)((128 - oled_text_width(a, 2)) / 2), 22, a, 2);
        const char *b = s.muted ? "READY - MUTED" : "READY";
        oled_text((int16_t)((128 - oled_text_width(b, 1)) / 2), 44, b, 1);
    } else {
        /* oldest pending call, large */
        oled_text(0, 20, "TABLE", 2);
        two_digits(buf, s.list[0]);
        oled_text(72, 13, buf, 4);

        snprintf(buf, sizeof(buf), "PENDING %u%s", (unsigned)s.count, s.muted ? "  MUTED" : "");
        oled_text(0, 43, buf, 1);

        /* remaining queue: "NEXT 05 07 09 +2" */
        if (s.count > 1) {
            char line[24] = "NEXT";
            uint8_t shown = 0;
            for (uint8_t i = 1; i < s.count && shown < 5; i++, shown++) {
                char t[4] = {' ', 0, 0, 0};
                two_digits(&t[1], s.list[i]);
                strcat(line, t);
            }
            if (s.count - 1 > shown) {
                char t[6];
                snprintf(t, sizeof(t), " +%u", (unsigned)(s.count - 1 - shown));
                strcat(line, t);
            }
            oled_text(0, 54, line, 1);
        }
    }

    /* transient message replaces the bottom row */
    if (s.msg != UI_MSG_NONE) {
        oled_invert_rect(0, 53, 128, 11);
        const char *m = "";
        char t[24];
        switch (s.msg) {
        case UI_MSG_HOLD_CLEAR:      m = "HOLD CLEAR 2S = ALL"; break;
        case UI_MSG_CLEARED:         snprintf(t, sizeof(t), "CLEARED %u CALLS", (unsigned)s.msg_arg); m = t; break;
        case UI_MSG_SERVED:          snprintf(t, sizeof(t), "TABLE %u SERVED", (unsigned)s.msg_arg); m = t; break;
        case UI_MSG_MUTED:           m = "SOUND MUTED"; break;
        case UI_MSG_UNMUTED:         m = "SOUND ON"; break;
        case UI_MSG_NOTHING_PENDING: m = "NOTHING PENDING"; break;
        }
        oled_text(2, 55, m, 1);
    }
}

static uint8_t g_page;           /* next page to render */
static bool    g_rendering;

void ui_update(const UiState &s)
{
    if (g_have_shown && memcmp(&g_shown, &s, sizeof(s)) == 0)
        return;                          /* nothing changed: no redraw, no flicker */
    g_shown = s;
    g_have_shown = true;
    g_page = 0;                          /* (re)start a full refresh from the new state */
    g_rendering = true;
}

void ui_service(void)
{
    if (!g_rendering || !oled_present())
        return;
    oled_set_page(g_page);               /* clears the 128-byte page buffer */
    draw(g_shown);                       /* drawing is clipped to the current page */
    oled_flush_page();
    if (++g_page >= 8)
        g_rendering = false;
}
