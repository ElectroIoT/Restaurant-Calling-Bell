#include <Arduino.h>
#include "config.h"
#include "button_handler.h"

void button_init(void)
{
#if BUTTON_EXTERNAL_PULLUP
    pinMode(PIN_BUTTON, INPUT);
#else
    pinMode(PIN_BUTTON, INPUT_PULLUP);
#endif
}

bool button_is_down(void)
{
    return digitalRead(PIN_BUTTON) == LOW;
}

bool button_confirm_press(void)
{
    if (!button_is_down())
        return false;
    delay(BUTTON_DEBOUNCE_MS);
    return button_is_down();
}

bool button_wait_release(unsigned long max_wait_ms)
{
    unsigned long t0 = millis();
    unsigned long up_since = 0;
    bool up = false;
    while ((unsigned long)(millis() - t0) < max_wait_ms) {
        if (!button_is_down()) {
            if (!up) { up = true; up_since = millis(); }
            if ((unsigned long)(millis() - up_since) >= BUTTON_DEBOUNCE_MS)
                return true;
        } else {
            up = false;
        }
    }
    return false;
}
