#include "rx_core.h"

static void make_reply(bell_packet_t *r, uint8_t type, const bell_packet_t *in, uint8_t status)
{
    r->type = type;
    r->table = in->table;
    r->seq = in->seq;
    r->epoch = in->epoch;
    r->status = status;
    r->battery = 0;
    r->flags = 0;
}

void rx_core_served_packet(uint8_t table, uint8_t epoch, uint8_t seq, bell_packet_t *out)
{
    out->type = BELL_MSG_SERVED;
    out->table = table;
    out->seq = seq;
    out->epoch = epoch;
    out->status = 0;
    out->battery = 0;
    out->flags = 0;
}

void rx_core_handle(call_table_t *t, const bell_packet_t *in, rx_action_t *out)
{
    out->send_reply = false;
    out->table_changed = false;
    out->announce = false;
    out->table = in->table;
    out->call_result = CT_RES_BAD_TABLE;

    switch (in->type) {
    case BELL_MSG_CALL: {
        ct_result_t res = ct_on_call(t, in->table, in->epoch, in->seq);
        out->call_result = res;
        switch (res) {
        case CT_RES_NEW:
        case CT_RES_REPEAT:
            out->table_changed = true;
            out->announce = true;
            out->send_reply = true;          /* reply first, persist after (caller order) */
            make_reply(&out->reply, BELL_MSG_ACCEPTED, in, BELL_ST_NEW);
            break;
        case CT_RES_DUP_PENDING:
            out->send_reply = true;          /* lost-ACK case: acknowledge again, create nothing */
            make_reply(&out->reply, BELL_MSG_ACCEPTED, in, BELL_ST_DUPLICATE);
            break;
        case CT_RES_DUP_SERVED:
            out->send_reply = true;
            make_reply(&out->reply, BELL_MSG_SERVED, in, 0);
            break;
        case CT_RES_BAD_TABLE:
        default:
            break;
        }
        break;
    }
    case BELL_MSG_STATUS_REQ: {
        ct_state_t st = ct_query(t, in->table, in->epoch, in->seq);
        out->send_reply = true;
        if (st == CT_PENDING)
            make_reply(&out->reply, BELL_MSG_ACCEPTED, in, BELL_ST_STILL_PENDING);
        else if (st == CT_SERVED)
            make_reply(&out->reply, BELL_MSG_SERVED, in, 0);
        else
            make_reply(&out->reply, BELL_MSG_ERROR, in, BELL_ERR_UNKNOWN_CALL);
        break;
    }
    default:
        break;                                /* replies/other message types are not for the receiver */
    }
}
