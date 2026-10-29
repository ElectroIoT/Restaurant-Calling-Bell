/*
 * call_table.h - receiver-side pending-call table (pure C, host-testable).
 *
 * One slot per table (1..BELL_MAX_TABLES), so a remote that keeps calling can never
 * occupy more than one queue entry and cannot starve the others. Display/ACK order is
 * FIFO by the time a table FIRST became pending (arrival order); a repeat call while
 * still pending keeps its place.
 *
 * A call is identified by (epoch, seq). Retries of one press carry the same pair.
 */
#ifndef CALL_TABLE_H
#define CALL_TABLE_H

#include "bell_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { CT_NONE = 0, CT_PENDING = 1, CT_SERVED = 2 } ct_state_t;

typedef enum {
    CT_RES_NEW,          /* table was idle/served: new pending call            */
    CT_RES_REPEAT,       /* new press while still pending: keeps its position  */
    CT_RES_DUP_PENDING,  /* retry of the call already pending                  */
    CT_RES_DUP_SERVED,   /* late retry of a call staff already served          */
    CT_RES_BAD_TABLE
} ct_result_t;

typedef struct {
    uint8_t  state;
    uint8_t  epoch;
    uint8_t  seq;
    uint16_t order;      /* arrival stamp, compared with wrap-safe subtraction */
} ct_slot_t;

typedef struct {
    ct_slot_t slot[BELL_MAX_TABLES];
    uint16_t  next_order;
} call_table_t;

/* Serialized form for EEPROM: 4 bytes per table: state, epoch, seq, rank(0 = oldest). */
#define CT_BLOB_SIZE (BELL_MAX_TABLES * 4u)

void        ct_init(call_table_t *t);
ct_result_t ct_on_call(call_table_t *t, uint8_t table, uint8_t epoch, uint8_t seq);

/* STATUS_REQ lookup: CT_PENDING / CT_SERVED if (epoch,seq) matches the stored call, else CT_NONE. */
ct_state_t  ct_query(const call_table_t *t, uint8_t table, uint8_t epoch, uint8_t seq);

uint8_t     ct_pending_count(const call_table_t *t);
uint8_t     ct_oldest_pending(const call_table_t *t);              /* 0 if none            */
uint8_t     ct_nth_pending(const call_table_t *t, uint8_t n);       /* 0-based FIFO, 0 if none */
bool        ct_is_pending(const call_table_t *t, uint8_t table);

/* Mark attended. Returns false if the table was not pending. */
bool        ct_serve(call_table_t *t, uint8_t table);
/* Serve the oldest pending call; returns its table or 0. */
uint8_t     ct_serve_oldest(call_table_t *t);
/* Administrative clear: every pending call becomes SERVED (silent). Returns how many. */
uint8_t     ct_clear_all(call_table_t *t);

void        ct_export(const call_table_t *t, uint8_t blob[CT_BLOB_SIZE]);
/* Returns false (and leaves the table empty) if the blob is inconsistent. */
bool        ct_import(call_table_t *t, const uint8_t blob[CT_BLOB_SIZE]);

#ifdef __cplusplus
}
#endif
#endif
