#include <string.h>
#include "test_common.h"
#include "remote_fsm.h"

static const rf_cfg_t CFG = { 8, 40, 60, 120 };

static bell_packet_t reply_pkt(uint8_t type, uint8_t table, uint8_t epoch, uint8_t seq, uint8_t status)
{
    bell_packet_t p = { type, table, seq, epoch, status, 0, 0 };
    return p;
}

/* Advance 1 ms at a time until the FSM wants to SEND or is DONE. Returns the action. */
static rf_action_t run_until(rf_t *f, uint32_t *now, uint32_t limit_ms)
{
    for (uint32_t i = 0; i < limit_ms; i++, (*now)++) {
        rf_action_t a = rf_poll(f, *now);
        if (a == RF_ACT_SEND || a == RF_ACT_DONE)
            return a;
    }
    return RF_ACT_WAIT;
}

int main(void)
{
    rf_t f;
    uint32_t now = 1000;
    bell_packet_t pk;

    /* 1. happy path: radio ACK then application reply */
    rf_start(&f, &CFG, BELL_MSG_CALL, 4, 9, 33, 0x1234);
    CHECK(rf_poll(&f, now) == RF_ACT_SEND);
    rf_build_packet(&f, 150, 0, &pk);
    CHECK(pk.type == BELL_MSG_CALL && pk.table == 4 && pk.seq == 33 && pk.epoch == 9);
    rf_sent(&f, now, true);
    CHECK(rf_poll(&f, now + 1) == RF_ACT_LISTEN);
    CHECK(f.result == RF_RES_NONE);                  /* radio ACK alone is NOT success */
    pk = reply_pkt(BELL_MSG_ACCEPTED, 4, 9, 33, BELL_ST_NEW);
    rf_on_packet(&f, &pk);
    CHECK(rf_poll(&f, now + 2) == RF_ACT_DONE && f.result == RF_RES_ACCEPTED);

    /* 2. reply lost: retry reuses the SAME seq, waits a bounded, jittered backoff */
    rf_start(&f, &CFG, BELL_MSG_CALL, 4, 9, 33, 0x1234);
    now = 5000;
    CHECK(rf_poll(&f, now) == RF_ACT_SEND);
    rf_sent(&f, now, true);
    uint32_t t_sent = now;
    CHECK(run_until(&f, &now, 500) == RF_ACT_SEND);
    uint32_t gap = now - t_sent;
    CHECK(gap >= CFG.reply_timeout_ms + CFG.backoff_base_ms);
    CHECK(gap <= (uint32_t)CFG.reply_timeout_ms + CFG.backoff_base_ms + CFG.backoff_jitter_ms + 2);
    rf_build_packet(&f, 150, 0, &pk);
    CHECK(pk.seq == 33 && pk.epoch == 9);            /* unchanged across retries */
    CHECK(f.attempt == 1);

    /* 3. nobody ever answers at radio level: exactly max_attempts sends, then NO_REPLY */
    rf_start(&f, &CFG, BELL_MSG_CALL, 2, 1, 7, 0xBEEF);
    now = 0;
    unsigned sends = 0;
    for (;;) {
        rf_action_t a = run_until(&f, &now, 1000);
        if (a == RF_ACT_DONE) break;
        CHECK(a == RF_ACT_SEND);
        sends++;
        rf_sent(&f, now, false);
        if (sends > 50) break;
    }
    CHECK(sends == CFG.max_attempts);
    CHECK(f.result == RF_RES_NO_REPLY);

    /* 4. radio ACK heard but application reply never comes: also bounded */
    rf_start(&f, &CFG, BELL_MSG_CALL, 2, 1, 7, 0xBEEF);
    now = 0; sends = 0;
    for (;;) {
        rf_action_t a = run_until(&f, &now, 1000);
        if (a == RF_ACT_DONE) break;
        sends++;
        rf_sent(&f, now, true);
        if (sends > 50) break;
    }
    CHECK(sends == CFG.max_attempts && f.result == RF_RES_NO_REPLY);

    /* 5. stale / foreign replies are ignored */
    rf_start(&f, &CFG, BELL_MSG_CALL, 4, 9, 33, 1);
    rf_poll(&f, 0); rf_sent(&f, 0, true);
    pk = reply_pkt(BELL_MSG_ACCEPTED, 4, 9, 32, 0); rf_on_packet(&f, &pk);   /* old seq */
    CHECK(f.result == RF_RES_NONE);
    pk = reply_pkt(BELL_MSG_ACCEPTED, 4, 8, 33, 0); rf_on_packet(&f, &pk);   /* old epoch */
    CHECK(f.result == RF_RES_NONE);
    pk = reply_pkt(BELL_MSG_ACCEPTED, 5, 9, 33, 0); rf_on_packet(&f, &pk);   /* other table */
    CHECK(f.result == RF_RES_NONE);
    pk = reply_pkt(BELL_MSG_CALL, 4, 9, 33, 0);     rf_on_packet(&f, &pk);   /* wrong type */
    CHECK(f.result == RF_RES_NONE);
    pk = reply_pkt(BELL_MSG_SERVED, 4, 9, 33, 0);   rf_on_packet(&f, &pk);
    CHECK(f.result == RF_RES_SERVED);

    /* 6. error and status-request outcomes */
    rf_start(&f, &CFG, BELL_MSG_CALL, 4, 9, 33, 1);
    rf_poll(&f, 0); rf_sent(&f, 0, true);
    pk = reply_pkt(BELL_MSG_ERROR, 4, 9, 33, BELL_ERR_BAD_TABLE); rf_on_packet(&f, &pk);
    CHECK(f.result == RF_RES_REJECTED);

    rf_start(&f, &CFG, BELL_MSG_STATUS_REQ, 4, 9, 33, 1);
    rf_poll(&f, 0); rf_sent(&f, 0, true);
    pk = reply_pkt(BELL_MSG_ACCEPTED, 4, 9, 33, BELL_ST_STILL_PENDING); rf_on_packet(&f, &pk);
    CHECK(f.result == RF_RES_PENDING);
    rf_start(&f, &CFG, BELL_MSG_STATUS_REQ, 4, 9, 33, 1);
    rf_poll(&f, 0); rf_sent(&f, 0, true);
    pk = reply_pkt(BELL_MSG_ERROR, 4, 9, 33, BELL_ERR_UNKNOWN_CALL); rf_on_packet(&f, &pk);
    CHECK(f.result == RF_RES_UNKNOWN);

    /* 7. millisecond counter rollover in the middle of a run */
    rf_start(&f, &CFG, BELL_MSG_CALL, 4, 9, 33, 77);
    now = 0xFFFFFFF0u; sends = 0;
    for (;;) {
        rf_action_t a = run_until(&f, &now, 1000);
        if (a == RF_ACT_DONE) break;
        sends++;
        rf_sent(&f, now, true);
        if (sends > 50) break;
    }
    CHECK(sends == CFG.max_attempts && f.result == RF_RES_NO_REPLY);
    CHECK(now < 0x10000u);                            /* we really crossed the wrap */

    /* 8. ten remotes started together get different backoffs (collision spreading) */
    uint32_t first_gap[10];
    for (int t = 1; t <= 10; t++) {
        rf_start(&f, &CFG, BELL_MSG_CALL, (uint8_t)t, 1, 1, 0x5555);
        now = 0;
        rf_poll(&f, now); rf_sent(&f, now, false);   /* instant failure -> backoff */
        run_until(&f, &now, 1000);
        first_gap[t - 1] = now;
    }
    int distinct = 0;
    for (int i = 0; i < 10; i++) {
        int seen = 0;
        for (int j = 0; j < i; j++) if (first_gap[j] == first_gap[i]) seen = 1;
        if (!seen) distinct++;
    }
    CHECK(distinct >= 6);

    TEST_MAIN_END("test_remote_fsm");
}
