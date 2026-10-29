#include <Arduino.h>
#include "config.h"
#include "led_status.h"
#include "power_manager.h"

static uint32_t g_last_toggle;
static bool g_state;

void led_init(void)
{
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);
}

void led_off(void)
{
    g_state = false;
    digitalWrite(PIN_LED, LOW);
}

void led_waiting_update(uint32_t now_ms)
{
    if ((int32_t)(now_ms - g_last_toggle) >= (int32_t)(LED_WAIT_PERIOD_MS / 2)) {
        g_last_toggle = now_ms;
        g_state = !g_state;
        digitalWrite(PIN_LED, g_state ? HIGH : LOW);
    }
}

static void blink(uint8_t n, uint16_t on_ms, uint16_t off_ms)
{
    for (uint8_t i = 0; i < n; i++) {
        digitalWrite(PIN_LED, HIGH);
        delay(on_ms);
        digitalWrite(PIN_LED, LOW);
        if (i + 1 < n)
            delay(off_ms);
    }
}

void led_success(void)
{
    digitalWrite(PIN_LED, HIGH);
    pm_sleep_ms(LED_SUCCESS_MS);                 /* pin stays driven while the MCU sleeps */
    led_off();
}

void led_failure(void)
{
    led_off();
    blink(LED_ERROR_FLASHES, 60, 60);
}

void led_served(void)
{
    led_off();
    blink(3, 300, 300);
}

void led_low_battery(void)
{
    delay(300);
    blink(2, 40, 120);
}

void led_fatal_radio(void)
{
    led_off();
    blink(1, 800, 0);
    delay(200);
    blink(3, 80, 120);
}
