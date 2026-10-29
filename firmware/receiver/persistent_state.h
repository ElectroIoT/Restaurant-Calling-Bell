/*
 * persistent_state.h - saves/restores the pending-call table across receiver power loss.
 * Backed by common/persist_log (wear-levelled ring, written only when the table changes).
 */
#ifndef RX_PERSISTENT_STATE_H
#define RX_PERSISTENT_STATE_H
#include "call_table.h"

/* Returns true if a valid saved table was restored into *t. */
bool persist_restore(call_table_t *t);
void persist_save(const call_table_t *t);
#endif
