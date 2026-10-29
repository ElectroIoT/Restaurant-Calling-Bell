/*
 * radio_driver.h - radio abstraction used by both firmwares.
 *
 * Application code only includes this header, never a register map or a third-party radio
 * library. The shipped implementation is radio_si24r1.cpp, a small register-level driver
 * written against the Si24R1 datasheet (see docs/radio-driver-choice.md for why RF24 and
 * libsi24 were not used). Another radio (or RF24 behind this API) can replace it by
 * providing the same functions.
 *
 * Model: static 8-byte payloads, 5-byte addresses, hardware auto-ACK + auto-retransmit.
 * Hardware auto-ACK only proves the far radio received the frame; application-level
 * acknowledgement is done by bell_protocol messages above this layer.
 */
#ifndef RADIO_DRIVER_H
#define RADIO_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

enum RadioRate  { RADIO_RATE_250K = 0, RADIO_RATE_1M = 1, RADIO_RATE_2M = 2 };

/* Si24R1 RF_PWR[2:0] codes. NOTE: this is NOT the nRF24L01 encoding. */
enum RadioPower {
    RADIO_PWR_M12DBM = 0, RADIO_PWR_M6DBM = 1, RADIO_PWR_M4DBM = 2, RADIO_PWR_0DBM = 3,
    RADIO_PWR_1DBM   = 4, RADIO_PWR_3DBM = 5,  RADIO_PWR_4DBM = 6,  RADIO_PWR_7DBM = 7
};

struct RadioConfig {
    uint8_t pin_ce;
    uint8_t pin_csn;
    uint8_t channel;        /* 0..125, f = 2400 + channel MHz */
    uint8_t rate;           /* RadioRate  */
    uint8_t power;          /* RadioPower */
    uint8_t retry_delay;    /* ARD code 0..15 -> (code+1)*250 us */
    uint8_t retry_count;    /* ARC 0..15 hardware retransmissions */
};

enum RadioTxResult { RADIO_TX_OK = 0, RADIO_TX_NO_ACK = 1, RADIO_TX_TIMEOUT = 2 };

/* Initialise SPI and the radio; returns false if the chip does not answer on SPI
 * (register write/read-back check). Leaves the radio powered down. */
bool radio_begin(const RadioConfig &cfg);

void radio_wake(void);          /* PWR_UP=1 and wait for the crystal (~2 ms), then standby */
void radio_sleep(void);         /* CE low + PWR_UP=0: shutdown, ~1 uA, registers retained  */
void radio_standby(void);       /* CE low, stay powered                                    */

/* Enter receive mode listening on `addr` (5 bytes, MSByte first). */
void radio_listen(const uint8_t addr[5]);
bool radio_rx_available(void);
/* Read one 8-byte payload. Returns false if the FIFO was empty. */
bool radio_rx_read(uint8_t buf[8]);
/* Drop everything in the RX FIFO (e.g. after switching mode). */
void radio_rx_flush(void);

/* Transmit one 8-byte payload to `dest`. Blocks for at most
 * (retry_count+1)*(retry_delay+1)*250us + airtime + margin (single-digit ms at defaults).
 * Leaves the radio in standby; call radio_listen() to resume reception. */
RadioTxResult radio_send(const uint8_t dest[5], const uint8_t payload[8]);

/* Diagnostics */
uint8_t radio_read_reg(uint8_t reg);
bool    radio_rssi_above_minus60(void);

#endif
