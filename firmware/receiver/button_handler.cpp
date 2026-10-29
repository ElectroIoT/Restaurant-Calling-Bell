#include <Arduino.h>
#include "config.h"
#include "button_handler.h"

struct Btn {
    uint8_t  pin;
    bool     raw;            /* last raw reading, true = pressed */
    bool     stable;         /* debounced state */
    uint32_t changed;        /* time the raw reading last changed */
    uint32_t pressed_at;
    bool     long_fired;
    bool     ev_press, ev_short, ev_long;
};

static Btn g_btn[BTN_COUNT] = {
    {PIN_BTN_ACK, false, false, 0, 0, false, false, false, false},
    {PIN_BTN_CLEAR, false, false, 0, 0, false, false, false, false},
    {PIN_BTN_MUTE, false, false, 0, 0, false, false, false, false},
};

void buttons_begin(void)
{
    for (uint8_t i = 0; i < BTN_COUNT; i++)
        pinMode(g_btn[i].pin, INPUT_PULLUP);
}

void buttons_update(uint32_t now)
{
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        Btn &b = g_btn[i];
        bool r = (digitalRead(b.pin) == LOW);
        if (r != b.raw) {
            b.raw = r;
            b.changed = now;
        }
        if (b.raw != b.stable && (uint32_t)(now - b.changed) >= BTN_DEBOUNCE_MS) {
            b.stable = b.raw;
            if (b.stable) {
                b.pressed_at = now;
                b.long_fired = false;
                b.ev_press = true;
            } else if (!b.long_fired) {
                b.ev_short = true;
            }
        }
        if (b.stable && !b.long_fired && (uint32_t)(now - b.pressed_at) >= CLEAR_HOLD_MS) {
            b.long_fired = true;
            b.ev_long = true;
        }
    }
}

static bool take(bool &flag)
{
    bool v = flag;
    flag = false;
    return v;
}

bool button_short_press(ButtonId b) { return take(g_btn[b].ev_short); }
bool button_long_press(ButtonId b)  { return take(g_btn[b].ev_long); }
bool button_press_edge(ButtonId b)  { return take(g_btn[b].ev_press); }
