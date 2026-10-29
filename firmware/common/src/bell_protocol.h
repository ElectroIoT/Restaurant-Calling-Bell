/*
 * bell_protocol.h - wire format for the restaurant calling bell (protocol v1).
 *
 * Every over-the-air payload is exactly 8 bytes (static payload length, no DPL):
 *
 *   byte 0  (version << 4) | type
 *   byte 1  table id (1..BELL_MAX_TABLES)
 *   byte 2  seq     - per-call sequence; every retry of one call reuses it
 *   byte 3  epoch   - per-boot counter of the remote; (epoch, seq) identifies a call
 *   byte 4  status  - meaning depends on type (see bell_status_t / bell_error_t)
 *   byte 5  battery - supply in units of 20 mV (0 = unknown). Remote -> receiver only.
 *   byte 6  flags   - bit0 = low battery
 *   byte 7  crc8    - CRC-8, poly 0x07, init 0x00, over bytes 0..6
 *
 * Multi-byte fields: none. All fields are single bytes, so byte order does not matter.
 * This file is plain C and has no hardware dependency (host-testable).
 */
#ifndef BELL_PROTOCOL_H
#define BELL_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BELL_PROTO_VERSION   1u
#define BELL_PACKET_SIZE     8u
#define BELL_MAX_TABLES      10u
#define BELL_ADDR_LEN        5u

typedef enum {
    BELL_MSG_CALL       = 1,  /* remote -> receiver: table wants service           */
    BELL_MSG_ACCEPTED   = 2,  /* receiver -> remote: call validated AND queued     */
    BELL_MSG_SERVED     = 3,  /* receiver -> remote: staff attended the call       */
    BELL_MSG_ERROR      = 4,  /* receiver -> remote: request refused (bell_error_t) */
    BELL_MSG_STATUS_REQ = 5   /* remote -> receiver: "is my call still pending?"   */
} bell_msg_type_t;

/* status byte of BELL_MSG_ACCEPTED */
typedef enum {
    BELL_ST_NEW           = 0,  /* queued as a new call                         */
    BELL_ST_DUPLICATE     = 1,  /* retry of a call that is already queued       */
    BELL_ST_STILL_PENDING = 2   /* reply to STATUS_REQ: still waiting for staff */
} bell_status_t;

/* status byte of BELL_MSG_ERROR */
typedef enum {
    BELL_ERR_BAD_TABLE    = 1,
    BELL_ERR_UNKNOWN_CALL = 2   /* STATUS_REQ for a call the receiver does not know */
} bell_error_t;

#define BELL_FLAG_LOW_BATT   0x01u

typedef struct {
    uint8_t type;      /* bell_msg_type_t */
    uint8_t table;     /* 1..BELL_MAX_TABLES */
    uint8_t seq;
    uint8_t epoch;
    uint8_t status;
    uint8_t battery;   /* x20 mV */
    uint8_t flags;
} bell_packet_t;

typedef enum {
    BELL_DECODE_OK = 0,
    BELL_DECODE_BAD_CRC,
    BELL_DECODE_BAD_VERSION,
    BELL_DECODE_BAD_TYPE,
    BELL_DECODE_BAD_TABLE
} bell_decode_t;

uint8_t       bell_crc8(const uint8_t *data, uint8_t len);
void          bell_encode(const bell_packet_t *pkt, uint8_t out[BELL_PACKET_SIZE]);
bell_decode_t bell_decode(const uint8_t in[BELL_PACKET_SIZE], bell_packet_t *pkt);

/* 5-byte pipe addresses, MSByte first as written in text.
 *   receiver:  "REST1"
 *   remote n:  "TAB" + two ASCII digits, e.g. table 3 -> "TAB03", table 10 -> "TAB10"
 * (The radio driver writes them LSByte first, as the SI24R1 requires.) */
void bell_addr_receiver(uint8_t out[BELL_ADDR_LEN]);
bool bell_addr_remote(uint8_t table, uint8_t out[BELL_ADDR_LEN]);

#ifdef __cplusplus
}
#endif
#endif
