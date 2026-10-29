/*
 * Restaurant calling bell - TABLE REMOTE (ATmega328P + SI24R1).
 *
 *   press -> CALL (retried, same seq) -> receiver validates + queues -> CALL_ACCEPTED
 *   LED: fast flash = waiting, 1 s solid = accepted, 5 rapid flashes = no acknowledgement
 *   after an accepted call the remote polls every 8 s (up to 5 min) and blinks 3x when
 *   the counter marks the call served.
 *
 * Idle = ATmega power-down + radio shutdown, woken by a pin-change interrupt on the button.
 * Configuration: config.h. Modules: button_handler, led_status, power_manager,
 * call_manager (+ common/remote_fsm), persistent_state, common/radio_driver.
 */
#include <Arduino.h>
#include "config.h"
#include "bell_protocol.h"
#include "radio_driver.h"
#include "call_manager.h"
#include "button_handler.h"
#include "led_status.h"
#include "power_manager.h"
#include "persistent_state.h"

static uint8_t  g_epoch;
static uint8_t  g_seq;
static bool     g_radio_ok;
static uint16_t g_poll_ticks_left;      /* 8-second ticks of served-polling remaining */
static uint8_t  g_last_seq;             /* seq of the call being polled */

#if BELL_DEBUG_SERIAL
#define DBG(x) Serial.println(F(x))
#else
#define DBG(x) do {} while (0)
#endif

static bool radio_start(void)
{
    const RadioConfig rc = {
        PIN_RADIO_CE, PIN_RADIO_CSN, BELL_CHANNEL, BELL_RATE, BELL_TX_POWER,
        BELL_HW_RETRY_DELAY, BELL_HW_RETRY_COUNT
    };
    return radio_begin(rc);
}

static void battery_info(uint8_t *b20mv, uint8_t *flags)
{
    uint16_t mv = pm_read_vcc_mv();
    uint16_t units = mv / 20u;
    *b20mv = (units > 255u) ? 255u : (uint8_t)units;
    *flags = (mv != 0 && mv < LOW_BATT_MV) ? BELL_FLAG_LOW_BATT : 0;
}

static void start_call(void)
{
    if (!g_radio_ok)
        g_radio_ok = radio_start();
    if (!g_radio_ok) {
        led_fatal_radio();                       /* never fail silently */
        return;
    }

    g_poll_ticks_left = 0;
    g_seq++;                                     /* one new seq per press; retries reuse it */
    g_last_seq = g_seq;

    uint8_t batt, flags;
    battery_info(&batt, &flags);

    rf_result_t res = call_run(BELL_MSG_CALL, TABLE_ID, g_epoch, g_seq, batt, flags, true);
    led_off();

    switch (res) {
    case RF_RES_ACCEPTED:
        DBG("accepted");
        led_success();
        if (SERVED_POLL_ENABLE)
            g_poll_ticks_left = SERVED_POLL_WINDOW_S / SERVED_POLL_PERIOD_S;
        break;
    case RF_RES_SERVED:
        DBG("accepted+served");
        led_success();
        break;
    default:
        DBG("no acknowledgement");
        led_failure();
        break;
    }
    if (flags & BELL_FLAG_LOW_BATT)
        led_low_battery();
}

/* One served-poll cycle. Returns true if polling should stop. */
static bool poll_once(void)
{
    if (!g_radio_ok)
        return true;
    uint8_t batt, flags;
    battery_info(&batt, &flags);
    rf_result_t res = call_run(BELL_MSG_STATUS_REQ, TABLE_ID, g_epoch, g_last_seq, batt, flags, false);
    switch (res) {
    case RF_RES_SERVED:
        led_served();
        return true;
    case RF_RES_UNKNOWN:
        /* Receiver has no record of our call (e.g. it lost power before saving): re-send it. */
        res = call_run(BELL_MSG_CALL, TABLE_ID, g_epoch, g_last_seq, batt, flags, false);
        return !(res == RF_RES_ACCEPTED);
    default:
        return false;                            /* pending, or no reply this time: try next tick */
    }
}

void setup()
{
    pm_init();
    led_init();
    button_init();
#if BELL_DEBUG_SERIAL
    Serial.begin(115200);
    DBG("remote boot");
#endif
    g_epoch = persistent_next_epoch();
    g_radio_ok = radio_start();
    if (g_radio_ok)
        radio_sleep();
    else
        led_fatal_radio();

    /* A button held at power-up (or stuck) must not trigger a call. */
    if (button_is_down())
        button_wait_release(BUTTON_STUCK_MS);
}

void loop()
{
    if (g_poll_ticks_left > 0) {
        pm_sleep_8s_or_button();
        if (button_is_down()) {                  /* a new press interrupts polling */
            g_poll_ticks_left = 0;
        } else {
            g_poll_ticks_left--;
            if (poll_once())
                g_poll_ticks_left = 0;
            return;
        }
    } else {
        pm_sleep_until_button();
    }

    if (!button_confirm_press())
        return;                                  /* glitch or release edge: back to sleep */

    start_call();

    /* One long press = one call. Wait for release; a button held for BUTTON_STUCK_MS is
     * treated as stuck and ignored (a stuck button then draws only the pull-up current). */
    button_wait_release(BUTTON_STUCK_MS);
    pm_sleep_ms(BUTTON_LOCKOUT_MS);              /* lockout without keeping the MCU awake */
}
