/*
 * remote_fsm.h - remote-side request/reply retry engine (pure C, host-testable).
 *
 * The FSM never touches hardware. The caller loop does:
 *
 *     switch (rf_poll(&f, now_ms)) {
 *       case RF_ACT_SEND:   ok = radio_send(...);  rf_sent(&f, now_ms, ok);  break;
 *       case RF_ACT_LISTEN: if (packet received) rf_on_packet(&f, &pkt);       break;
 *       case RF_ACT_WAIT:   idle / light sleep;                                break;
 *       case RF_ACT_DONE:   read f.result;                                     break;
 *     }
 *
 * One FSM run sends either a CALL or a STATUS_REQ and retries it (same seq every time)
 * until the receiver gives an application-level reply or attempts run out.
 * Radio-level auto-ACK is NOT treated as success: only a validated reply packet is.
 * All time maths is wrap-safe (uint32_t milliseconds, signed difference).
 */
#ifndef REMOTE_FSM_H
#define REMOTE_FSM_H

#include "bell_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t  max_attempts;        /* total transmissions per run                      */
    uint16_t reply_timeout_ms;    /* wait for the application reply after each send   */
    uint16_t backoff_base_ms;     /* minimum gap between attempts                     */
    uint16_t backoff_jitter_ms;   /* + pseudo-random 0..jitter-1 (collision spreading) */
} rf_cfg_t;

typedef enum { RF_ACT_SEND, RF_ACT_LISTEN, RF_ACT_WAIT, RF_ACT_DONE } rf_action_t;

typedef enum {
    RF_RES_NONE = 0,    /* still running                                              */
    RF_RES_ACCEPTED,    /* CALL: receiver validated and queued it (new or duplicate)   */
    RF_RES_SERVED,      /* receiver reports staff already attended (implies accepted)  */
    RF_RES_PENDING,     /* STATUS_REQ: still waiting for staff                         */
    RF_RES_UNKNOWN,     /* STATUS_REQ: receiver has no record -> caller should re-CALL */
    RF_RES_REJECTED,    /* receiver answered BELL_MSG_ERROR (bad table)                */
    RF_RES_NO_REPLY     /* attempts exhausted                                          */
} rf_result_t;

typedef enum { RF_ST_IDLE, RF_ST_SEND, RF_ST_WAIT_REPLY, RF_ST_BACKOFF, RF_ST_DONE } rf_state_t;

typedef struct {
    rf_cfg_t   cfg;
    rf_state_t state;
    rf_result_t result;
    uint8_t    msg_type;      /* BELL_MSG_CALL or BELL_MSG_STATUS_REQ */
    uint8_t    table, seq, epoch;
    uint8_t    attempt;       /* transmissions done so far */
    uint32_t   deadline;
    uint16_t   rng;
} rf_t;

void rf_start(rf_t *f, const rf_cfg_t *cfg, uint8_t msg_type,
              uint8_t table, uint8_t epoch, uint8_t seq, uint16_t rng_seed);
rf_action_t rf_poll(rf_t *f, uint32_t now_ms);
/* Report the outcome of the transmit started for RF_ACT_SEND (hw_ok = radio auto-ACK seen). */
void rf_sent(rf_t *f, uint32_t now_ms, bool hw_ok);
/* Feed a decoded packet received while listening. Ignores anything that is not our reply. */
void rf_on_packet(rf_t *f, const bell_packet_t *pkt);
/* Build the packet to transmit for the current attempt. */
void rf_build_packet(const rf_t *f, uint8_t battery, uint8_t flags, bell_packet_t *out);

#ifdef __cplusplus
}
#endif
#endif
