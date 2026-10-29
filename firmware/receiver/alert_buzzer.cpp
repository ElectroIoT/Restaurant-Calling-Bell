#include <Arduino.h>
#include "config.h"
#include "alert_buzzer.h"

#define BEEP_ON_MS   120
#define BEEP_OFF_MS  120

static uint8_t  g_pin;
static uint8_t  g_remaining;     /* beeps left, including the one in progress */
static bool     g_on;
static uint32_t g_edge;

void buzzer_begin(uint8_t pin)
{
    g_pin = pin;
    pinMode(g_pin, OUTPUT);
    digitalWrite(g_pin, LOW);
}

void buzzer_beeps(uint8_t count, bool muted)
{
#if BUZZER_ENABLE
    if (muted || count == 0)
        return;
    g_remaining = count;
    g_on = false;
    g_edge = millis() - BEEP_OFF_MS;      /* start immediately on the next service() */
#else
    (void)count; (void)muted;
#endif
}

void buzzer_service(uint32_t now)
{
    if (g_on) {
        if ((uint32_t)(now - g_edge) >= BEEP_ON_MS) {
            digitalWrite(g_pin, LOW);
            g_on = false;
            g_edge = now;
            if (g_remaining) g_remaining--;
        }
    } else if (g_remaining && (uint32_t)(now - g_edge) >= BEEP_OFF_MS) {
        digitalWrite(g_pin, HIGH);
        g_on = true;
        g_edge = now;
    }
}
