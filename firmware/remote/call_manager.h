#ifndef CALL_MANAGER_H
#define CALL_MANAGER_H
#include <stdint.h>
#include "remote_fsm.h"

/* Run one request (CALL or STATUS_REQ) to completion: radio wake, send/retry/listen loop,
 * radio back to shutdown. Flashes the LED while waiting if `flash_led` is set.
 * The radio must already be initialised with radio_begin(). */
rf_result_t call_run(uint8_t msg_type, uint8_t table, uint8_t epoch, uint8_t seq,
                     uint8_t battery_20mv, uint8_t flags, bool flash_led);
#endif
