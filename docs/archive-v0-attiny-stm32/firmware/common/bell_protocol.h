// Shared by every board: button node (ATtiny85), receiver (STM32C031), link tests (Nano).
// Keep this file free of board-specific includes.
#pragma once
#include <stdint.h>

#define BELL_CHANNEL        76          // 2476 MHz
#define BELL_ADDRESS        "BELL1"     // 5-byte pipe address, same on both sides
#define BELL_RETRY_DELAY    5           // x250us + 250us = 1500us between hardware retries
#define BELL_RETRY_COUNT    15

// Restaurant call types
enum BellCallType : uint8_t {
  CALL_NORMAL = 0,   // short press: waiter please
  CALL_BILL   = 1,   // long press: bill please
  CALL_URGENT = 2    // double press: urgent help
};

// ACK status sent back by the receiver
enum BellAckStatus : uint8_t {
  ACK_RECEIVED   = 0,
  ACK_QUEUED     = 1,   // behind other calls
  ACK_SERVED     = 2,
  ACK_QUEUE_FULL = 3
};

// flags bits
#define BELL_FLAG_LOW_BATT  0x01

// Button -> receiver, 6 bytes
struct __attribute__((packed)) BellCall {
  uint8_t table_id;   // 1..255, unique per button
  uint8_t type;       // BellCallType
  uint8_t seq;        // increments per press; retries keep the same value
  uint8_t battery;    // millivolts / 20
  uint8_t flags;
  uint8_t crc;        // crc8 of the first 5 bytes
};

// Receiver -> button, carried in the radio ACK, 2 bytes
struct __attribute__((packed)) BellAck {
  uint8_t status;     // BellAckStatus
  uint8_t crc;        // crc8 of status
};

// CRC-8, polynomial 0x07
static inline uint8_t bell_crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  while (len--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; i++)
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
  }
  return crc;
}

static inline void bell_call_seal(BellCall *c) {
  c->crc = bell_crc8((const uint8_t *)c, sizeof(BellCall) - 1);
}
static inline bool bell_call_valid(const BellCall *c) {
  return c->crc == bell_crc8((const uint8_t *)c, sizeof(BellCall) - 1);
}
static inline void bell_ack_seal(BellAck *a) {
  a->crc = bell_crc8((const uint8_t *)a, sizeof(BellAck) - 1);
}
static inline bool bell_ack_valid(const BellAck *a) {
  return a->crc == bell_crc8((const uint8_t *)a, sizeof(BellAck) - 1);
}
