#include <Arduino.h>
#include <avr/eeprom.h>
#include "persist_log.h"
#include "persistent_state.h"

static uint8_t ee_read(uint16_t a) { return eeprom_read_byte((const uint8_t *)a); }
static void ee_write(uint16_t a, uint8_t v) { eeprom_update_byte((uint8_t *)a, v); }

uint8_t persistent_next_epoch(void)
{
    plog_t log;
    plog_init(&log, ee_read, ee_write, 0, 1, 16);     /* 16 slots * 4 bytes = 64 bytes at EEPROM 0 */
    uint8_t epoch = 0;
    if (plog_load(&log, &epoch))
        epoch++;
    if (epoch == 0)
        epoch = 1;
    plog_save(&log, &epoch);
    return epoch;
}
