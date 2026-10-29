#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H
#include <stdbool.h>

void button_init(void);
bool button_is_down(void);            /* raw, active low */
/* True only if the button is still down after the debounce time (rejects glitches). */
bool button_confirm_press(void);
/* Block until released (debounced) so one long press yields exactly one call.
 * Gives up after max_wait_ms (stuck button) and returns false in that case. */
bool button_wait_release(unsigned long max_wait_ms);
#endif
