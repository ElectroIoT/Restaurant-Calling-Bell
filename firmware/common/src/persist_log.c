#include "persist_log.h"
#include "bell_protocol.h"   /* bell_crc8 */

#define ERASED 0xFFFFu

static uint16_t slot_size(const plog_t *p) { return (uint16_t)(p->blob_len + 3u); }
static uint16_t slot_addr(const plog_t *p, uint8_t slot) { return (uint16_t)(p->base + (uint16_t)slot * slot_size(p)); }

uint16_t plog_size(uint8_t blob_len, uint8_t slot_count)
{
    return (uint16_t)((uint16_t)slot_count * (uint16_t)(blob_len + 3u));
}

void plog_init(plog_t *p, uint8_t (*rd)(uint16_t), void (*wr)(uint16_t, uint8_t),
               uint16_t base, uint8_t blob_len, uint8_t slot_count)
{
    p->read = rd;
    p->write = wr;
    p->base = base;
    p->blob_len = blob_len;
    p->slot_count = slot_count;
    p->last_slot = 0;
    p->last_counter = 0;
    p->have_record = false;
}

/* Reads slot into buf (counter+blob), returns true if the CRC is valid and not erased. */
static bool read_slot(const plog_t *p, uint8_t slot, uint8_t *buf, uint16_t *counter)
{
    uint16_t a = slot_addr(p, slot);
    uint8_t n = (uint8_t)(p->blob_len + 2u);
    for (uint8_t i = 0; i < n; i++)
        buf[i] = p->read((uint16_t)(a + i));
    uint8_t crc = p->read((uint16_t)(a + n));
    *counter = (uint16_t)(buf[0] | ((uint16_t)buf[1] << 8));
    if (*counter == ERASED)
        return false;
    return bell_crc8(buf, n) == crc;
}

bool plog_load(plog_t *p, uint8_t *blob)
{
    uint8_t buf[PLOG_MAX_BLOB + 2u];
    uint16_t best = 0;
    bool found = false;
    uint8_t best_slot = 0;
    uint8_t best_buf[PLOG_MAX_BLOB + 2u];

    for (uint8_t s = 0; s < p->slot_count; s++) {
        uint16_t c;
        if (!read_slot(p, s, buf, &c))
            continue;
        if (!found || (int16_t)(uint16_t)(c - best) > 0) {
            found = true;
            best = c;
            best_slot = s;
            for (uint8_t i = 0; i < p->blob_len + 2u; i++)
                best_buf[i] = buf[i];
        }
    }
    p->have_record = found;
    if (!found)
        return false;
    p->last_slot = best_slot;
    p->last_counter = best;
    for (uint8_t i = 0; i < p->blob_len; i++)
        blob[i] = best_buf[i + 2u];
    return true;
}

void plog_save(plog_t *p, const uint8_t *blob)
{
    uint8_t slot = p->have_record ? (uint8_t)((p->last_slot + 1u) % p->slot_count) : 0u;
    uint16_t counter = (uint16_t)(p->last_counter + 1u);
    if (counter == ERASED || counter == 0u)
        counter = 1u;                       /* skip the erased marker */

    uint8_t buf[PLOG_MAX_BLOB + 2u];
    buf[0] = (uint8_t)(counter & 0xFFu);
    buf[1] = (uint8_t)(counter >> 8);
    for (uint8_t i = 0; i < p->blob_len; i++)
        buf[i + 2u] = blob[i];
    uint8_t n = (uint8_t)(p->blob_len + 2u);
    uint16_t a = slot_addr(p, slot);

    /* Invalidate the slot first (counter -> erased) so a torn write cannot look valid with a
     * stale CRC; then write payload, CRC, and finally the counter that makes it current. */
    p->write(a, 0xFF);
    p->write((uint16_t)(a + 1u), 0xFF);
    for (uint8_t i = 2; i < n; i++)
        p->write((uint16_t)(a + i), buf[i]);
    p->write((uint16_t)(a + n), bell_crc8(buf, n));
    p->write(a, buf[0]);
    p->write((uint16_t)(a + 1u), buf[1]);

    p->last_slot = slot;
    p->last_counter = counter;
    p->have_record = true;
}
