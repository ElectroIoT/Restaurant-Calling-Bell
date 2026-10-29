#include <string.h>
#include "test_common.h"
#include "persist_log.h"

static uint8_t g_ee[1024];
static unsigned g_writes_allowed;       /* simulate power loss: stop writing after N byte-writes */
static unsigned g_write_count;

static uint8_t rd(uint16_t a) { return g_ee[a]; }
static void wr(uint16_t a, uint8_t v)
{
    if (g_writes_allowed == 0) return;
    g_writes_allowed--;
    if (g_ee[a] != v) { g_ee[a] = v; g_write_count++; }   /* counts real cell writes */
}

int main(void)
{
    plog_t log;
    uint8_t blob[40], out[40];

    memset(g_ee, 0xFF, sizeof g_ee);
    g_writes_allowed = 0xFFFFFFFFu;
    plog_init(&log, rd, wr, 0, 40, 20);
    CHECK(!plog_load(&log, out));                     /* blank EEPROM */

    for (int i = 0; i < 40; i++) blob[i] = (uint8_t)i;
    plog_save(&log, blob);
    plog_t l2; plog_init(&l2, rd, wr, 0, 40, 20);
    CHECK(plog_load(&l2, out) && memcmp(out, blob, 40) == 0);

    /* many saves wrap the ring; newest always wins, including the counter wrapping past 65535 */
    for (unsigned n = 0; n < 70000; n++) {
        blob[0] = (uint8_t)n; blob[1] = (uint8_t)(n >> 8);
        plog_save(&log, blob);
        if (n % 9973 == 0 || n > 69990) {
            plog_t l3; plog_init(&l3, rd, wr, 0, 40, 20);
            CHECK(plog_load(&l3, out) && out[0] == blob[0] && out[1] == blob[1]);
        }
    }

    /* power cut at every possible byte during a save: the previous record must survive
     * (or the new one must be complete) - never garbage */
    uint8_t prev[40], next[40];
    for (int i = 0; i < 40; i++) { prev[i] = (uint8_t)(0x10 + i); next[i] = (uint8_t)(0x80 + i); }
    memset(g_ee, 0xFF, sizeof g_ee);
    g_writes_allowed = 0xFFFFFFFFu;
    plog_init(&log, rd, wr, 0, 40, 20);
    plog_save(&log, prev);
    plog_save(&log, prev);                           /* two records present */
    for (unsigned cut = 0; cut < 50; cut++) {
        uint8_t backup[1024]; memcpy(backup, g_ee, sizeof backup);
        plog_t a; plog_init(&a, rd, wr, 0, 40, 20); plog_load(&a, out);
        g_writes_allowed = cut;
        plog_save(&a, next);
        g_writes_allowed = 0xFFFFFFFFu;
        plog_t b; plog_init(&b, rd, wr, 0, 40, 20);
        CHECK(plog_load(&b, out));
        CHECK(memcmp(out, prev, 40) == 0 || memcmp(out, next, 40) == 0);
        memcpy(g_ee, backup, sizeof backup);          /* rewind for the next cut position */
    }

    /* corrupted newest record falls back to the previous one */
    memset(g_ee, 0xFF, sizeof g_ee);
    plog_init(&log, rd, wr, 0, 40, 20);
    plog_save(&log, prev);
    plog_save(&log, next);
    g_ee[43 + 5] ^= 0x01;                             /* flip a bit inside the newest slot (slot 1) */
    plog_t c; plog_init(&c, rd, wr, 0, 40, 20);
    CHECK(plog_load(&c, out) && memcmp(out, prev, 40) == 0);

    /* wear: a save whose payload barely changed costs few cell writes */
    memset(g_ee, 0xFF, sizeof g_ee);
    plog_init(&log, rd, wr, 0, 40, 20);
    for (int i = 0; i < 40; i++) plog_save(&log, prev);      /* fill the ring with identical data */
    g_write_count = 0;
    plog_save(&log, prev);
    CHECK(g_write_count <= 8);                               /* counter + CRC, not the whole slot */

    /* geometry */
    CHECK(plog_size(40, 20) == 860);

    TEST_MAIN_END("test_persist_log");
}
