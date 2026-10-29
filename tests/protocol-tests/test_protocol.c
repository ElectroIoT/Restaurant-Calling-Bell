#include <string.h>
#include "test_common.h"
#include "bell_protocol.h"

int main(void)
{
    bell_packet_t p = { BELL_MSG_CALL, 7, 0xA5, 0x3C, BELL_ST_NEW, 150, BELL_FLAG_LOW_BATT }, q;
    uint8_t raw[BELL_PACKET_SIZE];

    /* exact wire layout from docs/wireless-protocol.md */
    bell_encode(&p, raw);
    CHECK(raw[0] == 0x11);              /* version 1, type CALL */
    CHECK(raw[1] == 7 && raw[2] == 0xA5 && raw[3] == 0x3C);
    CHECK(raw[4] == 0 && raw[5] == 150 && raw[6] == 1);
    CHECK(raw[7] == bell_crc8(raw, 7));
    CHECK(bell_decode(raw, &q) == BELL_DECODE_OK);
    CHECK(memcmp(&p, &q, sizeof p) == 0);

    /* CRC-8 poly 0x07 check value for "123456789" is 0xF4 */
    CHECK(bell_crc8((const uint8_t *)"123456789", 9) == 0xF4);

    /* every single-bit corruption is detected */
    for (int byte = 0; byte < 8; byte++)
        for (int bit = 0; bit < 8; bit++) {
            uint8_t c[8]; memcpy(c, raw, 8); c[byte] ^= (uint8_t)(1u << bit);
            CHECK(bell_decode(c, &q) != BELL_DECODE_OK);
        }

    /* validation errors (re-seal the CRC so only the field is wrong) */
    uint8_t c[8];
    memcpy(c, raw, 8); c[0] = (uint8_t)((2 << 4) | 1); c[7] = bell_crc8(c, 7);
    CHECK(bell_decode(c, &q) == BELL_DECODE_BAD_VERSION);
    memcpy(c, raw, 8); c[0] = (uint8_t)((1 << 4) | 9); c[7] = bell_crc8(c, 7);
    CHECK(bell_decode(c, &q) == BELL_DECODE_BAD_TYPE);
    memcpy(c, raw, 8); c[0] = (uint8_t)(1 << 4); c[7] = bell_crc8(c, 7);
    CHECK(bell_decode(c, &q) == BELL_DECODE_BAD_TYPE);       /* type 0 invalid */
    memcpy(c, raw, 8); c[1] = 0; c[7] = bell_crc8(c, 7);
    CHECK(bell_decode(c, &q) == BELL_DECODE_BAD_TABLE);
    memcpy(c, raw, 8); c[1] = 11; c[7] = bell_crc8(c, 7);
    CHECK(bell_decode(c, &q) == BELL_DECODE_BAD_TABLE);

    /* addresses */
    uint8_t a[5];
    bell_addr_receiver(a);
    CHECK(memcmp(a, "REST1", 5) == 0);
    CHECK(bell_addr_remote(3, a) && memcmp(a, "TAB03", 5) == 0);
    CHECK(bell_addr_remote(10, a) && memcmp(a, "TAB10", 5) == 0);
    CHECK(!bell_addr_remote(0, a) && !bell_addr_remote(11, a));
    /* all remote addresses are distinct from each other and from the receiver */
    uint8_t all[11][5];
    bell_addr_receiver(all[0]);
    for (int t = 1; t <= 10; t++) bell_addr_remote((uint8_t)t, all[t]);
    for (int i = 0; i < 11; i++)
        for (int j = i + 1; j < 11; j++)
            CHECK(memcmp(all[i], all[j], 5) != 0);

    TEST_MAIN_END("test_protocol");
}
