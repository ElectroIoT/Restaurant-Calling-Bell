#include <Arduino.h>
#include <avr/eeprom.h>
#include "config.h"
#include "persist_log.h"
#include "persistent_state.h"

static plog_t g_log;
static bool   g_inited;

static uint8_t ee_read(uint16_t a) { return eeprom_read_byte((const uint8_t *)a); }
static void ee_write(uint16_t a, uint8_t v) { eeprom_update_byte((uint8_t *)a, v); }

static void init_log(void)
{
    if (g_inited)
        return;
    plog_init(&g_log, ee_read, ee_write, PERSIST_EEPROM_BASE, CT_BLOB_SIZE, PERSIST_SLOTS);
    g_inited = true;
}

bool persist_restore(call_table_t *t)
{
    init_log();
    uint8_t blob[CT_BLOB_SIZE];
    if (!plog_load(&g_log, blob))
        return false;
    return ct_import(t, blob);
}

void persist_save(const call_table_t *t)
{
    init_log();
    uint8_t blob[CT_BLOB_SIZE];
    ct_export(t, blob);
    plog_save(&g_log, blob);
}
