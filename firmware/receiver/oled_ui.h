#ifndef OLED_UI_H
#define OLED_UI_H
#include <stdint.h>
#include <stdbool.h>

enum UiMessage : uint8_t {
    UI_MSG_NONE = 0,
    UI_MSG_HOLD_CLEAR,      /* short CLEAR press: explain the long-press */
    UI_MSG_CLEARED,         /* all calls cleared (arg = count) */
    UI_MSG_SERVED,          /* ACK pressed (arg = table) */
    UI_MSG_MUTED,
    UI_MSG_UNMUTED,
    UI_MSG_NOTHING_PENDING
};

struct UiState {
    uint8_t count;          /* pending calls */
    uint8_t list[10];       /* pending tables in FIFO order (oldest first), count valid entries */
    bool    muted;
    bool    radio_ok;
    bool    restored;       /* queue was restored from EEPROM at boot */
    uint8_t msg;            /* UiMessage */
    uint8_t msg_arg;
};

/* Redraw only if `s` differs from what is on screen. Cheap to call every loop. */
void ui_update(const UiState &s);
/* Render and send ONE 8-pixel page of a pending refresh (a few ms). Call every loop. */
void ui_service(void);
#endif
