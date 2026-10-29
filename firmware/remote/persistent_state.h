#ifndef PERSISTENT_STATE_H
#define PERSISTENT_STATE_H
#include <stdint.h>
/* Per-boot epoch (1 byte, wear-levelled ring in EEPROM, written once per boot).
 * (epoch, seq) identifies a call, so a restarted remote that reuses seq 0 is never mistaken
 * for a duplicate of its previous life. */
uint8_t persistent_next_epoch(void);
#endif
