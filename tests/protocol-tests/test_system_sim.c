/*
 * 10-remote + receiver simulation over a lossy, collision-prone channel.
 *
 * Uses the REAL firmware logic: remote_fsm (retry engine), bell_encode/decode (wire format) and
 * rx_core (the receiver's decision code). Only the radio is modelled:
 *   - uplink loss, downlink (reply) loss, radio-ACK loss (frame delivered but the remote does not
 *     hear the hardware ACK, so it retries -> receiver sees a duplicate),
 *   - collisions: two remotes sending in the same millisecond both lose,
 *   - a remote only hears the reply while it is in its listening window.
 * This validates protocol logic, NOT RF range or timing on real silicon.
 */
#include <string.h>
#include "test_common.h"
#include "rx_core.h"
#include "remote_fsm.h"

#define N 10
typedef struct {
    rf_t f; uint8_t table, epoch, seq, msg; uint32_t start;
    bool started, done; rf_result_t res; unsigned sends;
} rem_t;
typedef struct { bool used; uint32_t due; int rem; bell_packet_t pkt; } pend_t;

static call_table_t g_ct;
static unsigned g_new[N + 1][256];      /* NEW/REPEAT events per (table, seq) */
static unsigned g_dups, g_collisions, g_delivered;

static bool chance(unsigned pct) { return (test_rand() % 100u) < pct; }

static void simulate(rem_t *r, int n, uint32_t t0, unsigned up, unsigned down, unsigned ackloss, uint32_t max_ms)
{
    static const rf_cfg_t cfg = { 8, 40, 60, 120 };
    pend_t q[64];
    memset(q, 0, sizeof q);

    for (uint32_t ms = 0; ms < max_ms; ms++) {
        uint32_t now = t0 + ms;                     /* wraps naturally when t0 is near 2^32 */

        for (int i = 0; i < 64; i++) {
            if (q[i].used && (int32_t)(now - q[i].due) >= 0) {
                q[i].used = false;
                rem_t *x = &r[q[i].rem];
                if (x->started && !x->done && x->f.state == RF_ST_WAIT_REPLY)
                    rf_on_packet(&x->f, &q[i].pkt);   /* heard only while listening */
            }
        }

        int tx[N], ntx = 0, active = 0;
        for (int i = 0; i < n; i++) {
            rem_t *x = &r[i];
            if (x->done) continue;
            active++;
            if (!x->started) {
                if ((int32_t)(now - (t0 + x->start)) < 0) continue;
                rf_start(&x->f, &cfg, x->msg, x->table, x->epoch, x->seq, (uint16_t)test_rand());
                x->started = true;
            }
            rf_action_t a = rf_poll(&x->f, now);
            if (a == RF_ACT_SEND) tx[ntx++] = i;
            else if (a == RF_ACT_DONE) { x->done = true; x->res = x->f.result; }
        }
        if (active == 0) return;

        if (ntx > 1) {                               /* collision: everybody involved loses */
            g_collisions += (unsigned)ntx;
            for (int k = 0; k < ntx; k++) { r[tx[k]].sends++; rf_sent(&r[tx[k]].f, now, false); }
        } else if (ntx == 1) {
            rem_t *x = &r[tx[0]];
            x->sends++;
            bool hw_ok = false;
            if (!chance(up)) {
                bell_packet_t in, dec;
                uint8_t raw[BELL_PACKET_SIZE];
                rf_build_packet(&x->f, 150, 0, &in);
                bell_encode(&in, raw);
                CHECK(bell_decode(raw, &dec) == BELL_DECODE_OK);
                rx_action_t act;
                rx_core_handle(&g_ct, &dec, &act);
                g_delivered++;
                if (dec.type == BELL_MSG_CALL) {
                    if (act.call_result == CT_RES_NEW || act.call_result == CT_RES_REPEAT)
                        g_new[dec.table][dec.seq]++;
                    if (act.call_result == CT_RES_DUP_PENDING || act.call_result == CT_RES_DUP_SERVED)
                        g_dups++;
                }
                if (act.send_reply && !chance(down)) {
                    for (int i = 0; i < 64; i++)
                        if (!q[i].used) { q[i].used = true; q[i].due = now + 3; q[i].rem = tx[0]; q[i].pkt = act.reply; break; }
                }
                hw_ok = !chance(ackloss);
            }
            rf_sent(&x->f, now, hw_ok);
        }
    }
}

static void setup_remotes(rem_t *r, uint8_t msg, unsigned spread_ms)
{
    for (int i = 0; i < N; i++) {
        r[i].table = (uint8_t)(i + 1);
        r[i].msg = msg;
        r[i].start = spread_ms ? test_rand() % spread_ms : 0;
        r[i].started = r[i].done = false;
        r[i].sends = 0;
        r[i].res = RF_RES_NONE;
    }
}

static int count_ok(const rem_t *r)
{
    int ok = 0;
    for (int i = 0; i < N; i++)
        if (r[i].res == RF_RES_ACCEPTED || r[i].res == RF_RES_SERVED) ok++;
    return ok;
}

int main(void)
{
    rem_t r[N];
    ct_init(&g_ct);

    /* ---- A: all ten press within 3 ms (forced collisions) on a lossy channel, clock about to wrap ---- */
    memset(r, 0, sizeof r);
    setup_remotes(r, BELL_MSG_CALL, 3);
    for (int i = 0; i < N; i++) { r[i].epoch = (uint8_t)(1 + i); r[i].seq = (uint8_t)(200 + i * 7); }
    simulate(r, N, 0xFFFFFF00u, 15, 15, 10, 8000);
    int ok = count_ok(r);
    printf("  A: %d/10 accepted, %u deliveries, %u collisions, %u duplicates\n", ok, g_delivered, g_collisions, g_dups);
    CHECK(ok == 10);
    for (int i = 0; i < N; i++) {
        CHECK(r[i].done);
        CHECK(g_new[r[i].table][r[i].seq] == 1);                       /* exactly one queue entry per press */
        CHECK(ct_query(&g_ct, r[i].table, r[i].epoch, r[i].seq) == CT_PENDING);
    }
    CHECK(ct_pending_count(&g_ct) == 10);
    CHECK(g_collisions > 0);                                           /* the stress actually happened */

    /* ---- B: radio-ACK loss on every other frame: duplicates arrive, none create entries ---- */
    ct_init(&g_ct);
    memset(g_new, 0, sizeof g_new);
    g_dups = 0;
    setup_remotes(r, BELL_MSG_CALL, 400);
    simulate(r, N, 5000, 0, 0, 60, 20000);
    printf("  B: %d/10 accepted, duplicates seen %u\n", count_ok(r), g_dups);
    CHECK(g_dups > 0);
    for (int i = 0; i < N; i++)
        CHECK(g_new[r[i].table][r[i].seq] <= 1);
    CHECK(ct_pending_count(&g_ct) == (uint8_t)count_ok(r) || ct_pending_count(&g_ct) >= (uint8_t)count_ok(r));

    /* ---- C: receiver power-cycle before persistence: STATUS_REQ -> UNKNOWN -> re-CALL recovers ---- */
    rem_t saved[N];
    memcpy(saved, r, sizeof r);
    ct_init(&g_ct);                                                   /* queue lost */
    memset(g_new, 0, sizeof g_new);
    for (int i = 0; i < N; i++) {
        r[i].msg = BELL_MSG_STATUS_REQ;
        r[i].start = test_rand() % 200;
        r[i].started = r[i].done = false;
        r[i].res = RF_RES_NONE;
    }
    simulate(r, N, 100, 5, 5, 5, 8000);
    int unknown = 0;
    for (int i = 0; i < N; i++) if (r[i].res == RF_RES_UNKNOWN) unknown++;
    CHECK(unknown >= 8);
    for (int i = 0; i < N; i++) {                                     /* remotes re-send the same call */
        r[i].msg = BELL_MSG_CALL;
        r[i].start = test_rand() % 200;
        r[i].started = r[i].done = false;
        r[i].res = RF_RES_NONE;
    }
    simulate(r, N, 20000, 5, 5, 5, 8000);
    int recovered = 0;
    for (int i = 0; i < N; i++)
        if (ct_query(&g_ct, r[i].table, r[i].epoch, r[i].seq) == CT_PENDING) recovered++;
    printf("  C: %d unknown replies, %d calls recovered after receiver restart\n", unknown, recovered);
    CHECK(recovered >= 9);

    /* ---- D: staff serve everything; polls now report SERVED, late retries create nothing ---- */
    unsigned served = 0;
    while (ct_serve_oldest(&g_ct)) served++;
    CHECK(served == (unsigned)recovered);
    memset(g_new, 0, sizeof g_new);
    for (int i = 0; i < N; i++) {
        r[i].msg = BELL_MSG_STATUS_REQ;
        r[i].start = test_rand() % 100;
        r[i].started = r[i].done = false;
        r[i].res = RF_RES_NONE;
    }
    simulate(r, N, 50000, 0, 0, 0, 3000);
    int got_served = 0;
    for (int i = 0; i < N; i++) if (r[i].res == RF_RES_SERVED) got_served++;
    CHECK(got_served == recovered);
    for (int i = 0; i < N; i++) {
        r[i].msg = BELL_MSG_CALL;                                     /* stale retry of an already-served call */
        r[i].start = test_rand() % 100;
        r[i].started = r[i].done = false;
        r[i].res = RF_RES_NONE;
    }
    simulate(r, N, 60000, 0, 0, 0, 3000);
    unsigned late_new = 0;
    for (int t = 1; t <= N; t++) for (int s = 0; s < 256; s++) late_new += g_new[t][s];
    CHECK(late_new == 0);
    CHECK(ct_pending_count(&g_ct) == 0);

    (void)saved;
    TEST_MAIN_END("test_system_sim");
}
