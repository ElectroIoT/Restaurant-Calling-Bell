#include "bell_protocol.h"

uint8_t bell_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++)
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x07u) : (uint8_t)(crc << 1);
    }
    return crc;
}

void bell_encode(const bell_packet_t *p, uint8_t out[BELL_PACKET_SIZE])
{
    out[0] = (uint8_t)((BELL_PROTO_VERSION << 4) | (p->type & 0x0Fu));
    out[1] = p->table;
    out[2] = p->seq;
    out[3] = p->epoch;
    out[4] = p->status;
    out[5] = p->battery;
    out[6] = p->flags;
    out[7] = bell_crc8(out, BELL_PACKET_SIZE - 1u);
}

bell_decode_t bell_decode(const uint8_t in[BELL_PACKET_SIZE], bell_packet_t *p)
{
    if (bell_crc8(in, BELL_PACKET_SIZE - 1u) != in[7])
        return BELL_DECODE_BAD_CRC;
    if ((in[0] >> 4) != BELL_PROTO_VERSION)
        return BELL_DECODE_BAD_VERSION;
    uint8_t type = in[0] & 0x0Fu;
    if (type < BELL_MSG_CALL || type > BELL_MSG_STATUS_REQ)
        return BELL_DECODE_BAD_TYPE;
    if (in[1] < 1u || in[1] > BELL_MAX_TABLES)
        return BELL_DECODE_BAD_TABLE;
    p->type    = type;
    p->table   = in[1];
    p->seq     = in[2];
    p->epoch   = in[3];
    p->status  = in[4];
    p->battery = in[5];
    p->flags   = in[6];
    return BELL_DECODE_OK;
}

void bell_addr_receiver(uint8_t out[BELL_ADDR_LEN])
{
    out[0] = 'R'; out[1] = 'E'; out[2] = 'S'; out[3] = 'T'; out[4] = '1';
}

bool bell_addr_remote(uint8_t table, uint8_t out[BELL_ADDR_LEN])
{
    if (table < 1u || table > BELL_MAX_TABLES)
        return false;
    out[0] = 'T'; out[1] = 'A'; out[2] = 'B';
    out[3] = (uint8_t)('0' + table / 10u);
    out[4] = (uint8_t)('0' + table % 10u);
    return true;
}
