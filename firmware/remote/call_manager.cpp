#include <Arduino.h>
#include "config.h"
#include "bell_protocol.h"
#include "radio_driver.h"
#include "call_manager.h"
#include "led_status.h"

rf_result_t call_run(uint8_t msg_type, uint8_t table, uint8_t epoch, uint8_t seq,
                     uint8_t battery_20mv, uint8_t flags, bool flash_led)
{
    uint8_t rx_addr[BELL_ADDR_LEN];     /* where the receiver sends its reply: our own address */
    uint8_t tx_addr[BELL_ADDR_LEN];     /* receiver */
    bell_addr_remote(table, rx_addr);
    bell_addr_receiver(tx_addr);

    const rf_cfg_t cfg = {
        CALL_MAX_ATTEMPTS, CALL_REPLY_TIMEOUT_MS, CALL_BACKOFF_BASE_MS, CALL_BACKOFF_JITTER_MS
    };
    rf_t f;
    /* Seed from the microsecond counter: press timing differs between remotes. */
    rf_start(&f, &cfg, msg_type, table, epoch, seq, (uint16_t)micros());

    radio_wake();

    for (;;) {
        uint32_t now = millis();
        if (flash_led)
            led_waiting_update(now);

        switch (rf_poll(&f, now)) {
        case RF_ACT_SEND: {
            bell_packet_t pkt;
            uint8_t raw[BELL_PACKET_SIZE];
            rf_build_packet(&f, battery_20mv, flags, &pkt);
            bell_encode(&pkt, raw);
            RadioTxResult r = radio_send(tx_addr, raw);
            rf_sent(&f, millis(), r == RADIO_TX_OK);
            if (r == RADIO_TX_OK)
                radio_listen(rx_addr);          /* receiver's application reply comes to us */
            break;
        }
        case RF_ACT_LISTEN:
            if (radio_rx_available()) {
                uint8_t raw[BELL_PACKET_SIZE];
                bell_packet_t pkt;
                if (radio_rx_read(raw) && bell_decode(raw, &pkt) == BELL_DECODE_OK)
                    rf_on_packet(&f, &pkt);
            }
            break;
        case RF_ACT_WAIT:
            radio_standby();                    /* RX draws ~14 mA: do not listen while backing off */
            break;
        case RF_ACT_DONE:
            radio_sleep();
            return f.result;
        }
    }
}
