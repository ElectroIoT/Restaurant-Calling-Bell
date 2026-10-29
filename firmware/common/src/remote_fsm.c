#include "remote_fsm.h"

static bool reached(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

/* xorshift16; never returns to 0 as long as the seed is non-zero. */
static uint16_t rng_next(uint16_t *s)
{
    uint16_t x = *s;
    x ^= (uint16_t)(x << 7);
    x ^= (uint16_t)(x >> 9);
    x ^= (uint16_t)(x << 8);
    *s = x;
    return x;
}

void rf_start(rf_t *f, const rf_cfg_t *cfg, uint8_t msg_type,
              uint8_t table, uint8_t epoch, uint8_t seq, uint16_t rng_seed)
{
    f->cfg = *cfg;
    f->state = RF_ST_SEND;
    f->result = RF_RES_NONE;
    f->msg_type = msg_type;
    f->table = table;
    f->epoch = epoch;
    f->seq = seq;
    f->attempt = 0;
    f->deadline = 0;
    /* Mix the table id in so ten remotes started by the same event diverge. */
    f->rng = (uint16_t)(rng_seed ^ (uint16_t)(table * 0x9E37u));
    if (f->rng == 0)
        f->rng = 0xACE1u;
}

rf_action_t rf_poll(rf_t *f, uint32_t now_ms)
{
    switch (f->state) {
    case RF_ST_SEND:
        return RF_ACT_SEND;
    case RF_ST_WAIT_REPLY:
        if (reached(now_ms, f->deadline)) {
            /* No application reply in time: retry or give up. */
            if (f->attempt >= f->cfg.max_attempts) {
                f->result = RF_RES_NO_REPLY;
                f->state = RF_ST_DONE;
                return RF_ACT_DONE;
            }
            uint16_t jitter = f->cfg.backoff_jitter_ms ? (uint16_t)(rng_next(&f->rng) % f->cfg.backoff_jitter_ms) : 0u;
            f->deadline = now_ms + f->cfg.backoff_base_ms + jitter;
            f->state = RF_ST_BACKOFF;
            return RF_ACT_WAIT;
        }
        return RF_ACT_LISTEN;
    case RF_ST_BACKOFF:
        if (reached(now_ms, f->deadline)) {
            f->state = RF_ST_SEND;
            return RF_ACT_SEND;
        }
        return RF_ACT_WAIT;
    case RF_ST_DONE:
        return RF_ACT_DONE;
    case RF_ST_IDLE:
    default:
        return RF_ACT_DONE;
    }
}

void rf_sent(rf_t *f, uint32_t now_ms, bool hw_ok)
{
    if (f->state != RF_ST_SEND)
        return;
    f->attempt++;
    if (hw_ok) {
        /* Receiver radio heard us; its application reply should follow shortly. */
        f->deadline = now_ms + f->cfg.reply_timeout_ms;
        f->state = RF_ST_WAIT_REPLY;
    } else {
        /* Nobody acknowledged at radio level: skip the reply wait, go straight to backoff. */
        if (f->attempt >= f->cfg.max_attempts) {
            f->result = RF_RES_NO_REPLY;
            f->state = RF_ST_DONE;
            return;
        }
        uint16_t jitter = f->cfg.backoff_jitter_ms ? (uint16_t)(rng_next(&f->rng) % f->cfg.backoff_jitter_ms) : 0u;
        f->deadline = now_ms + f->cfg.backoff_base_ms + jitter;
        f->state = RF_ST_BACKOFF;
    }
}

void rf_on_packet(rf_t *f, const bell_packet_t *pkt)
{
    if (f->state != RF_ST_WAIT_REPLY && f->state != RF_ST_BACKOFF && f->state != RF_ST_SEND)
        return;
    if (pkt->table != f->table || pkt->epoch != f->epoch || pkt->seq != f->seq)
        return;                                  /* stale reply for an older call */

    switch (pkt->type) {
    case BELL_MSG_ACCEPTED:
        f->result = (pkt->status == BELL_ST_STILL_PENDING && f->msg_type == BELL_MSG_STATUS_REQ)
                        ? RF_RES_PENDING : RF_RES_ACCEPTED;
        break;
    case BELL_MSG_SERVED:
        f->result = RF_RES_SERVED;
        break;
    case BELL_MSG_ERROR:
        f->result = (pkt->status == BELL_ERR_UNKNOWN_CALL) ? RF_RES_UNKNOWN : RF_RES_REJECTED;
        break;
    default:
        return;
    }
    f->state = RF_ST_DONE;
}

void rf_build_packet(const rf_t *f, uint8_t battery, uint8_t flags, bell_packet_t *out)
{
    out->type = f->msg_type;
    out->table = f->table;
    out->seq = f->seq;
    out->epoch = f->epoch;
    out->status = 0;
    out->battery = battery;
    out->flags = flags;
}
