#include <string.h>
#include "test_common.h"
#include "call_table.h"

int main(void)
{
    call_table_t t;
    ct_init(&t);
    CHECK(ct_pending_count(&t) == 0 && ct_oldest_pending(&t) == 0);

    /* new call, retries are duplicates and never add entries */
    CHECK(ct_on_call(&t, 3, 1, 10) == CT_RES_NEW);
    CHECK(ct_on_call(&t, 3, 1, 10) == CT_RES_DUP_PENDING);
    CHECK(ct_on_call(&t, 3, 1, 10) == CT_RES_DUP_PENDING);
    CHECK(ct_pending_count(&t) == 1);

    /* FIFO by first arrival */
    CHECK(ct_on_call(&t, 7, 1, 1) == CT_RES_NEW);
    CHECK(ct_on_call(&t, 2, 5, 9) == CT_RES_NEW);
    CHECK(ct_nth_pending(&t, 0) == 3 && ct_nth_pending(&t, 1) == 7 && ct_nth_pending(&t, 2) == 2);
    CHECK(ct_nth_pending(&t, 3) == 0);

    /* a new press from a still-pending table keeps its place (no queue jumping) */
    CHECK(ct_on_call(&t, 3, 1, 11) == CT_RES_REPEAT);
    CHECK(ct_pending_count(&t) == 3 && ct_oldest_pending(&t) == 3);

    /* one remote hammering new presses never takes a second slot */
    for (int i = 0; i < 300; i++) ct_on_call(&t, 7, 1, (uint8_t)(i + 2));
    CHECK(ct_pending_count(&t) == 3);
    CHECK(ct_nth_pending(&t, 1) == 7);

    /* status queries */
    CHECK(ct_query(&t, 2, 5, 9) == CT_PENDING);
    CHECK(ct_query(&t, 2, 5, 8) == CT_NONE);        /* different seq */
    CHECK(ct_query(&t, 2, 6, 9) == CT_NONE);        /* different epoch */
    CHECK(ct_query(&t, 5, 1, 1) == CT_NONE);

    /* serve oldest */
    CHECK(ct_serve_oldest(&t) == 3);
    CHECK(ct_query(&t, 3, 1, 11) == CT_SERVED);
    CHECK(ct_on_call(&t, 3, 1, 11) == CT_RES_DUP_SERVED);   /* late retry after service */
    CHECK(ct_pending_count(&t) == 2 && ct_oldest_pending(&t) == 7);
    CHECK(!ct_serve(&t, 3));                                /* already served */
    CHECK(ct_serve(&t, 2));
    CHECK(ct_oldest_pending(&t) == 7);

    /* after service the table can call again as NEW and goes to the back */
    CHECK(ct_on_call(&t, 3, 1, 12) == CT_RES_NEW);
    CHECK(ct_nth_pending(&t, 0) == 7 && ct_nth_pending(&t, 1) == 3);

    /* restart of a remote: new epoch, same seq -> not a duplicate */
    ct_serve(&t, 3);
    CHECK(ct_on_call(&t, 3, 2, 12) == CT_RES_NEW);

    /* bad table */
    CHECK(ct_on_call(&t, 0, 1, 1) == CT_RES_BAD_TABLE);
    CHECK(ct_on_call(&t, 11, 1, 1) == CT_RES_BAD_TABLE);

    /* export/import preserves order and state */
    uint8_t blob[CT_BLOB_SIZE];
    ct_export(&t, blob);
    call_table_t u;
    CHECK(ct_import(&u, blob));
    CHECK(ct_pending_count(&u) == ct_pending_count(&t));
    for (uint8_t i = 0; i < 4; i++) CHECK(ct_nth_pending(&u, i) == ct_nth_pending(&t, i));
    CHECK(ct_on_call(&u, 9, 1, 1) == CT_RES_NEW);           /* new call goes after restored ones */
    CHECK(ct_nth_pending(&u, (uint8_t)(ct_pending_count(&u) - 1)) == 9);
    CHECK(ct_on_call(&u, 7, 1, 100) == CT_RES_REPEAT);

    /* corrupt blobs are rejected and leave an empty table */
    ct_export(&t, blob);
    blob[0] = 9;
    CHECK(!ct_import(&u, blob) && ct_pending_count(&u) == 0);
    ct_export(&t, blob);
    for (int i = 0; i < 10; i++)
        if (blob[i * 4] == CT_PENDING) { blob[i * 4 + 3] = 99; break; }
    CHECK(!ct_import(&u, blob));

    /* clear-all marks everything served (polls then see SERVED, not UNKNOWN) */
    uint8_t n = ct_pending_count(&t);
    uint8_t first = ct_oldest_pending(&t);
    uint8_t first_epoch = t.slot[first - 1].epoch, first_seq = t.slot[first - 1].seq;
    CHECK(ct_clear_all(&t) == n && ct_pending_count(&t) == 0);
    CHECK(ct_query(&t, first, first_epoch, first_seq) == CT_SERVED);

    /* arrival counter wrap: order stays correct across 65536 calls */
    ct_init(&t);
    for (unsigned i = 0; i < 70000; i++) {
        uint8_t tb = (uint8_t)(1 + i % 10);
        ct_on_call(&t, tb, 1, (uint8_t)i);
        ct_serve(&t, tb);
    }
    ct_on_call(&t, 4, 1, 1); ct_on_call(&t, 8, 1, 1); ct_on_call(&t, 2, 1, 1);
    CHECK(ct_nth_pending(&t, 0) == 4 && ct_nth_pending(&t, 1) == 8 && ct_nth_pending(&t, 2) == 2);

    TEST_MAIN_END("test_call_table");
}
