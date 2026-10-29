#include "call_table.h"

static bool valid_table(uint8_t table)
{
    return table >= 1u && table <= BELL_MAX_TABLES;
}

void ct_init(call_table_t *t)
{
    for (uint8_t i = 0; i < BELL_MAX_TABLES; i++) {
        t->slot[i].state = CT_NONE;
        t->slot[i].epoch = 0;
        t->slot[i].seq   = 0;
        t->slot[i].order = 0;
    }
    t->next_order = 0;
}

ct_result_t ct_on_call(call_table_t *t, uint8_t table, uint8_t epoch, uint8_t seq)
{
    if (!valid_table(table))
        return CT_RES_BAD_TABLE;
    ct_slot_t *s = &t->slot[table - 1u];

    bool same_call = (s->state != CT_NONE) && s->epoch == epoch && s->seq == seq;
    if (same_call)
        return (s->state == CT_PENDING) ? CT_RES_DUP_PENDING : CT_RES_DUP_SERVED;

    ct_result_t res;
    if (s->state == CT_PENDING) {
        res = CT_RES_REPEAT;              /* keep arrival order */
    } else {
        s->order = t->next_order++;
        res = CT_RES_NEW;
    }
    s->state = CT_PENDING;
    s->epoch = epoch;
    s->seq   = seq;
    return res;
}

ct_state_t ct_query(const call_table_t *t, uint8_t table, uint8_t epoch, uint8_t seq)
{
    if (!valid_table(table))
        return CT_NONE;
    const ct_slot_t *s = &t->slot[table - 1u];
    if (s->state != CT_NONE && s->epoch == epoch && s->seq == seq)
        return (ct_state_t)s->state;
    return CT_NONE;
}

uint8_t ct_pending_count(const call_table_t *t)
{
    uint8_t n = 0;
    for (uint8_t i = 0; i < BELL_MAX_TABLES; i++)
        if (t->slot[i].state == CT_PENDING)
            n++;
    return n;
}

bool ct_is_pending(const call_table_t *t, uint8_t table)
{
    return valid_table(table) && t->slot[table - 1u].state == CT_PENDING;
}

/* "a is older than b" with a wrap-safe 16-bit comparison. */
static bool older(uint16_t a, uint16_t b)
{
    return (int16_t)(uint16_t)(a - b) < 0;
}

uint8_t ct_nth_pending(const call_table_t *t, uint8_t n)
{
    /* Selection by rank: the n-th oldest = the pending slot with exactly n older pending slots. */
    for (uint8_t i = 0; i < BELL_MAX_TABLES; i++) {
        if (t->slot[i].state != CT_PENDING)
            continue;
        uint8_t older_count = 0;
        for (uint8_t j = 0; j < BELL_MAX_TABLES; j++)
            if (j != i && t->slot[j].state == CT_PENDING && older(t->slot[j].order, t->slot[i].order))
                older_count++;
        if (older_count == n)
            return (uint8_t)(i + 1u);
    }
    return 0;
}

uint8_t ct_oldest_pending(const call_table_t *t)
{
    return ct_nth_pending(t, 0);
}

bool ct_serve(call_table_t *t, uint8_t table)
{
    if (!ct_is_pending(t, table))
        return false;
    t->slot[table - 1u].state = CT_SERVED;
    return true;
}

uint8_t ct_serve_oldest(call_table_t *t)
{
    uint8_t table = ct_oldest_pending(t);
    if (table)
        t->slot[table - 1u].state = CT_SERVED;
    return table;
}

uint8_t ct_clear_all(call_table_t *t)
{
    uint8_t n = 0;
    for (uint8_t i = 0; i < BELL_MAX_TABLES; i++) {
        if (t->slot[i].state == CT_PENDING) {
            t->slot[i].state = CT_SERVED;
            n++;
        }
    }
    return n;
}

void ct_export(const call_table_t *t, uint8_t blob[CT_BLOB_SIZE])
{
    for (uint8_t i = 0; i < BELL_MAX_TABLES; i++) {
        uint8_t rank = 0;
        if (t->slot[i].state == CT_PENDING) {
            for (uint8_t j = 0; j < BELL_MAX_TABLES; j++)
                if (j != i && t->slot[j].state == CT_PENDING && older(t->slot[j].order, t->slot[i].order))
                    rank++;
        }
        blob[i * 4u + 0u] = t->slot[i].state;
        blob[i * 4u + 1u] = t->slot[i].epoch;
        blob[i * 4u + 2u] = t->slot[i].seq;
        blob[i * 4u + 3u] = rank;
    }
}

bool ct_import(call_table_t *t, const uint8_t blob[CT_BLOB_SIZE])
{
    ct_init(t);
    uint8_t pending = 0;
    for (uint8_t i = 0; i < BELL_MAX_TABLES; i++) {
        if (blob[i * 4u] > CT_SERVED) {
            ct_init(t);
            return false;
        }
        if (blob[i * 4u] == CT_PENDING)
            pending++;
    }
    for (uint8_t i = 0; i < BELL_MAX_TABLES; i++) {
        t->slot[i].state = blob[i * 4u + 0u];
        t->slot[i].epoch = blob[i * 4u + 1u];
        t->slot[i].seq   = blob[i * 4u + 2u];
        t->slot[i].order = blob[i * 4u + 3u];
        if (t->slot[i].state == CT_PENDING && t->slot[i].order >= pending) {
            ct_init(t);
            return false;
        }
    }
    t->next_order = pending;   /* ranks 0..pending-1 are in use; new calls go after them */
    return true;
}
