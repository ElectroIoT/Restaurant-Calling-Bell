/*
 * persist_log.h - wear-levelled append-only record ring over a byte store (EEPROM).
 *
 * Slot layout: [counter_lo][counter_hi][blob ...][crc8 over everything before it].
 * counter 0xFFFF marks an erased slot. Saving writes the NEXT slot with counter+1, so a
 * power cut during a save leaves the previous record intact (its CRC still validates and
 * the half-written slot fails CRC). Load picks the valid slot with the newest counter
 * (wrap-safe). Writes go through a callback that must only write bytes that changed
 * (avr-libc eeprom_update_byte does), which keeps wear low.
 *
 * Pure C, host-testable: the EEPROM is accessed only through the two callbacks.
 */
#ifndef PERSIST_LOG_H
#define PERSIST_LOG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLOG_MAX_BLOB 48u

typedef struct {
    uint8_t (*read)(uint16_t addr);
    void    (*write)(uint16_t addr, uint8_t value);   /* write only if different */
    uint16_t base;          /* first EEPROM address used */
    uint8_t  blob_len;      /* 1..PLOG_MAX_BLOB */
    uint8_t  slot_count;    /* >= 2 */
    /* runtime state */
    uint8_t  last_slot;
    uint16_t last_counter;
    bool     have_record;
} plog_t;

void plog_init(plog_t *p, uint8_t (*rd)(uint16_t), void (*wr)(uint16_t, uint8_t),
               uint16_t base, uint8_t blob_len, uint8_t slot_count);
/* Scan the ring. Returns true and fills blob if a valid record exists. */
bool plog_load(plog_t *p, uint8_t *blob);
/* Append a new record. */
void plog_save(plog_t *p, const uint8_t *blob);
/* Bytes of store used by a ring of this geometry. */
uint16_t plog_size(uint8_t blob_len, uint8_t slot_count);

#ifdef __cplusplus
}
#endif
#endif
