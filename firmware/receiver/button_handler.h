/*
 * button_handler.h - debounced counter buttons (ACK, CLEAR, MUTE), all active low.
 * Events are latched until read. Long-press fires once while the button is still held,
 * so a noisy glitch or a short tap can never trigger "clear all".
 */
#ifndef RX_BUTTON_HANDLER_H
#define RX_BUTTON_HANDLER_H
#include <stdint.h>
#include <stdbool.h>

enum ButtonId : uint8_t { BTN_ACK = 0, BTN_CLEAR = 1, BTN_MUTE = 2, BTN_COUNT = 3 };

void buttons_begin(void);
void buttons_update(uint32_t now_ms);
bool button_short_press(ButtonId b);   /* released before the long-press time; clears the latch */
bool button_long_press(ButtonId b);    /* held >= hold time; clears the latch */
bool button_press_edge(ButtonId b);    /* debounced press (immediately); clears the latch */
#endif
