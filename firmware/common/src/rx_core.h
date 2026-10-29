/*
 * rx_core.h - the receiver's per-packet decision logic, separated from hardware so the exact
 * code that runs on the ATmega328P is also exercised by the host tests and the 10-remote
 * simulation (tests/protocol-tests/test_system_sim.c).
 *
 * Input: one decoded packet from a remote. Output: what to send back and which side effects
 * (display, announcement, EEPROM save) the caller must perform.
 */
#ifndef RX_CORE_H
#define RX_CORE_H

#include "bell_protocol.h"
#include "call_table.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool          send_reply;      /* transmit `reply` to the remote of reply.table            */
    bell_packet_t reply;
    bool          table_changed;   /* call table modified: schedule an EEPROM save             */
    bool          announce;        /* a call became (or was re-)pending: voice + buzzer       */
    uint8_t       table;           /* table the packet concerned                               */
    ct_result_t   call_result;     /* valid for BELL_MSG_CALL                                  */
} rx_action_t;

void rx_core_handle(call_table_t *t, const bell_packet_t *in, rx_action_t *out);

/* Reply packet telling the remote its call was served (sent when staff press ACK). */
void rx_core_served_packet(uint8_t table, uint8_t epoch, uint8_t seq, bell_packet_t *out);

#ifdef __cplusplus
}
#endif
#endif
