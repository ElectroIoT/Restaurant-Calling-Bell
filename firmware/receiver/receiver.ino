/*
 * Restaurant calling bell - COUNTER RECEIVER (ATmega328P + SI24R1 + SSD1306 + DFPlayer Mini).
 *
 * Loop (no long blocking delays; worst blocking step is one radio reply ~8 ms or one EEPROM save):
 *   1. drain the radio RX FIFO (max 3 packets per pass, FIFO order = fair),
 *      validate -> call_table -> reply CALL_ACCEPTED/SERVED/ERROR over the air
 *   2. buttons: ACK = serve oldest (+ CALL_SERVED to that remote), CLEAR = hold 2 s, MUTE = toggle
 *   3. repeat unanswered announcements, audio queue, buzzer, OLED (one page per pass)
 *   4. persist the call table when it changed, radio health check, watchdog
 *
 * Modules: common/{bell_protocol,call_table,persist_log,radio_driver}, ssd1306 + oled_ui,
 * audio_player, alert_buzzer, button_handler, persistent_state.
 */
#include <Arduino.h>
#include <avr/wdt.h>
#include "config.h"
#include "bell_protocol.h"
#include "call_table.h"
#include "rx_core.h"
#include "radio_driver.h"
#include "ssd1306.h"
#include "oled_ui.h"
#include "audio_player.h"
#include "alert_buzzer.h"
#include "button_handler.h"
#include "persistent_state.h"

#if !BELL_BOARD_PCB
#include <SoftwareSerial.h>
static SoftwareSerial g_audio_serial(PIN_AUDIO_SOFT_RX, PIN_AUDIO_SOFT_TX);
#endif

#if BELL_DEBUG_SERIAL
#define DBG_PRINT(...)   Serial.print(__VA_ARGS__)
#define DBG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DBG_PRINT(...)   do {} while (0)
#define DBG_PRINTLN(...) do {} while (0)
#endif

static call_table_t g_ct;
static bool     g_ct_dirty;                       /* table changed, EEPROM not yet updated */
static uint32_t g_last_announce[BELL_MAX_TABLES];
static uint8_t  g_repeats[BELL_MAX_TABLES];
static bool     g_radio_ok;
static bool     g_restored;
static uint8_t  g_ui_msg;
static uint8_t  g_ui_msg_arg;
static uint32_t g_ui_msg_until;
static uint32_t g_last_health;
static uint32_t g_dup_count, g_rx_count, g_bad_count;

static uint8_t g_rx_addr[BELL_ADDR_LEN];

static bool radio_start(void)
{
    const RadioConfig rc = {
        PIN_RADIO_CE, PIN_RADIO_CSN, BELL_CHANNEL, BELL_RATE, BELL_TX_POWER,
        BELL_HW_RETRY_DELAY, BELL_HW_RETRY_COUNT
    };
    if (!radio_begin(rc))
        return false;
    radio_wake();
    radio_listen(g_rx_addr);
    return true;
}

static void announce(uint8_t table, uint32_t now)
{
    audio_enqueue((uint16_t)(AUDIO_TRACK_TABLE_BASE + table - 1));
    g_last_announce[table - 1] = now;
}

static void ui_message(uint8_t msg, uint8_t arg, uint32_t now)
{
    g_ui_msg = msg;
    g_ui_msg_arg = arg;
    g_ui_msg_until = now + UI_MESSAGE_MS;
}

/* Send one protocol message to a remote and go back to listening. Bounded by the radio driver. */
static RadioTxResult reply(uint8_t type, uint8_t table, uint8_t epoch, uint8_t seq, uint8_t status)
{
    bell_packet_t p = { type, table, seq, epoch, status, 0, 0 };
    uint8_t raw[BELL_PACKET_SIZE], dest[BELL_ADDR_LEN];
    bell_encode(&p, raw);
    bell_addr_remote(table, dest);
    RadioTxResult r = radio_send(dest, raw);
    radio_listen(g_rx_addr);
    return r;
}

static void handle_packet(const bell_packet_t &p, uint32_t now)
{
    rx_action_t act;
    rx_core_handle(&g_ct, &p, &act);              /* shared, host-tested decision logic */

    if (p.type == BELL_MSG_CALL) {
        DBG_PRINT(F("CALL table ")); DBG_PRINT(p.table); DBG_PRINT(F(" seq ")); DBG_PRINT(p.seq);
        DBG_PRINT(F(" epoch ")); DBG_PRINT(p.epoch); DBG_PRINT(F(" batt ")); DBG_PRINT(p.battery * 20);
        DBG_PRINT(F("mV res ")); DBG_PRINTLN((int)act.call_result);
        if (act.call_result == CT_RES_DUP_PENDING || act.call_result == CT_RES_DUP_SERVED)
            g_dup_count++;
    }
    if (act.announce) {
        g_repeats[p.table - 1] = 0;
        announce(p.table, now);
        buzzer_beeps(2, audio_is_muted());
    }
    if (act.table_changed)
        g_ct_dirty = true;                        /* saved at the end of loop(), after the reply */
    /* Reply right away; the EEPROM save follows. A call lost in that window is recovered by the
     * remote's STATUS_REQ poll (UNKNOWN_CALL -> it re-sends the CALL). */
    if (act.send_reply)
        reply(act.reply.type, act.reply.table, act.reply.epoch, act.reply.seq, act.reply.status);
}

static void service_radio(uint32_t now)
{
    /* At most 3 packets per pass (RX FIFO depth): other work is never starved by a burst. */
    for (uint8_t n = 0; n < 3 && radio_rx_available(); n++) {
        uint8_t raw[BELL_PACKET_SIZE];
        bell_packet_t p;
        if (!radio_rx_read(raw))
            break;
        g_rx_count++;
        bell_decode_t d = bell_decode(raw, &p);
        if (d != BELL_DECODE_OK) {
            g_bad_count++;
            DBG_PRINT(F("bad packet ")); DBG_PRINTLN((int)d);
            continue;
        }
        handle_packet(p, now);
    }

    if ((uint32_t)(now - g_last_health) >= RADIO_HEALTH_CHECK_MS) {
        g_last_health = now;
        /* A brown-out/glitch resets the SI24R1 to defaults: detect and re-initialise. */
        if (!g_radio_ok || radio_read_reg(0x05) != BELL_CHANNEL)
            g_radio_ok = radio_start();
    }
}

static void service_buttons(uint32_t now)
{
    buttons_update(now);

    if (button_press_edge(BTN_ACK)) {
        uint8_t t = ct_oldest_pending(&g_ct);
        if (t == 0) {
            ui_message(UI_MSG_NOTHING_PENDING, 0, now);
        } else {
            uint8_t epoch = g_ct.slot[t - 1].epoch, seq = g_ct.slot[t - 1].seq;
            ct_serve(&g_ct, t);
            g_ct_dirty = true;
            ui_message(UI_MSG_SERVED, t, now);
#if AUDIO_PLAY_SERVED_TONE
            audio_enqueue(AUDIO_TRACK_SERVED);
#endif
            /* Best effort: the remote may be asleep; its STATUS_REQ poll will get SERVED later. */
            reply(BELL_MSG_SERVED, t, epoch, seq, 0);
        }
    }

    if (button_long_press(BTN_CLEAR)) {
        uint8_t n = ct_clear_all(&g_ct);
        g_ct_dirty = true;
        ui_message(UI_MSG_CLEARED, n, now);
    }
    if (button_short_press(BTN_CLEAR))
        ui_message(UI_MSG_HOLD_CLEAR, 0, now);

    if (button_short_press(BTN_MUTE)) {
        bool m = !audio_is_muted();
        audio_set_mute(m);
        ui_message(m ? UI_MSG_MUTED : UI_MSG_UNMUTED, 0, now);
    }
    (void)button_long_press(BTN_ACK);
    (void)button_long_press(BTN_MUTE);
    (void)button_short_press(BTN_ACK);
    (void)button_press_edge(BTN_CLEAR);
    (void)button_press_edge(BTN_MUTE);
}

static void service_repeats(uint32_t now)
{
    for (uint8_t t = 1; t <= BELL_MAX_TABLES; t++) {
        if (!ct_is_pending(&g_ct, t))
            continue;
        if (REPEAT_MAX != 0 && g_repeats[t - 1] >= REPEAT_MAX)
            continue;
        if ((uint32_t)(now - g_last_announce[t - 1]) >= (uint32_t)REPEAT_INTERVAL_S * 1000UL) {
            if (audio_queue_len() < AUDIO_QUEUE_LEN - 1) {
                g_repeats[t - 1]++;
                announce(t, now);
                DBG_PRINT(F("repeat table ")); DBG_PRINTLN(t);
            }
        }
    }
}

static void service_ui(uint32_t now)
{
    if (g_ui_msg != UI_MSG_NONE && (int32_t)(now - g_ui_msg_until) >= 0)
        g_ui_msg = UI_MSG_NONE;

    UiState s;
    memset(&s, 0, sizeof(s));
    s.count = ct_pending_count(&g_ct);
    for (uint8_t i = 0; i < s.count; i++)
        s.list[i] = ct_nth_pending(&g_ct, i);
    s.muted = audio_is_muted();
    s.radio_ok = g_radio_ok;
    s.restored = g_restored;
    s.msg = g_ui_msg;
    s.msg_arg = g_ui_msg_arg;
    ui_update(s);
    ui_service();
}

void setup()
{
#if BELL_ENABLE_WDT
    wdt_enable(WDTO_4S);
#endif
#if BELL_DEBUG_SERIAL
    Serial.begin(115200);
    Serial.println(F("receiver boot"));
#endif
    bell_addr_receiver(g_rx_addr);

    ct_init(&g_ct);
    g_restored = persist_restore(&g_ct);
    DBG_PRINT(F("restored ")); DBG_PRINTLN(g_restored ? ct_pending_count(&g_ct) : 0);

    buttons_begin();
    buzzer_begin(PIN_BUZZER);
    oled_begin(OLED_I2C_ADDR);
    DBG_PRINTLN(oled_present() ? F("oled ok") : F("oled NOT FOUND"));

#if BELL_BOARD_PCB
    Serial.begin(9600);
    audio_begin(Serial, PIN_AUDIO_BUSY, AUDIO_VOLUME);
#else
    g_audio_serial.begin(9600);
    audio_begin(g_audio_serial, PIN_AUDIO_BUSY, AUDIO_VOLUME);
#endif

    g_radio_ok = radio_start();
    DBG_PRINTLN(g_radio_ok ? F("radio ok") : F("radio NOT FOUND"));

    /* Calls restored from EEPROM are announced again once, so staff notice after a power cut. */
    uint32_t now = millis();
    for (uint8_t t = 1; t <= BELL_MAX_TABLES; t++)
        if (ct_is_pending(&g_ct, t))
            announce(t, now);
    audio_enqueue(AUDIO_TRACK_READY);
}

void loop()
{
    uint32_t now = millis();
#if BELL_ENABLE_WDT
    wdt_reset();
#endif
    service_radio(now);
    service_buttons(now);
    service_repeats(now);
    audio_service(now);
    buzzer_service(now);
    service_ui(now);

    if (g_ct_dirty) {                            /* after the radio work, so replies are never delayed */
        persist_save(&g_ct);
        g_ct_dirty = false;
    }
}
