/*
 * radio_si24r1.cpp - register-level Si24R1 driver (Arduino/AVR, polling, no IRQ pin needed).
 * Register map and SPI commands: docs/datasheets/Si24R1-datasheet-V1-eng.pdf sections 5 and 6.
 *
 * Differences from the nRF24L01+ that matter here:
 *   - RF_PWR is a 3-bit field RF_SETUP[2:0] (nRF24 uses 2 bits at [2:1]); codes differ.
 *   - Data rate bits are RF_DR_LOW (bit5) / RF_DR_HIGH (bit3): 00=1M, 01=2M, 10=250k.
 *   - Shutdown-to-standby takes 1.5-2 ms; registers/FIFOs are retained in shutdown.
 * Only static payloads (8 bytes) are used, so DPL / ACK payloads are not needed.
 */
#include <Arduino.h>
#include <SPI.h>
#include "radio_driver.h"

/* SPI commands */
#define CMD_R_REGISTER   0x00
#define CMD_W_REGISTER   0x20
#define CMD_R_RX_PAYLOAD 0x61
#define CMD_W_TX_PAYLOAD 0xA0
#define CMD_FLUSH_TX     0xE1
#define CMD_FLUSH_RX     0xE2
#define CMD_NOP          0xFF

/* Registers */
#define REG_CONFIG       0x00
#define REG_EN_AA        0x01
#define REG_EN_RXADDR    0x02
#define REG_SETUP_AW     0x03
#define REG_SETUP_RETR   0x04
#define REG_RF_CH        0x05
#define REG_RF_SETUP     0x06
#define REG_STATUS       0x07
#define REG_OBSERVE_TX   0x08
#define REG_RSSI         0x09
#define REG_RX_ADDR_P0   0x0A
#define REG_RX_ADDR_P1   0x0B
#define REG_TX_ADDR      0x10
#define REG_RX_PW_P0     0x11
#define REG_RX_PW_P1     0x12
#define REG_FIFO_STATUS  0x17
#define REG_DYNPD        0x1C
#define REG_FEATURE      0x1D

/* CONFIG bits */
#define CFG_EN_CRC   0x08
#define CFG_CRCO     0x04   /* 2-byte CRC */
#define CFG_PWR_UP   0x02
#define CFG_PRIM_RX  0x01

/* STATUS bits */
#define ST_RX_DR     0x40
#define ST_TX_DS     0x20
#define ST_MAX_RT    0x10

#define PAYLOAD_LEN  8
#define ADDR_LEN     5

static RadioConfig g_cfg;
static const SPISettings kSpi(2000000, MSBFIRST, SPI_MODE0);

static inline void csn_low(void)  { digitalWrite(g_cfg.pin_csn, LOW); }
static inline void csn_high(void) { digitalWrite(g_cfg.pin_csn, HIGH); }

static uint8_t spi_cmd(uint8_t cmd)
{
    SPI.beginTransaction(kSpi);
    csn_low();
    uint8_t st = SPI.transfer(cmd);
    csn_high();
    SPI.endTransaction();
    return st;
}

static void write_reg(uint8_t reg, uint8_t value)
{
    SPI.beginTransaction(kSpi);
    csn_low();
    SPI.transfer(CMD_W_REGISTER | (reg & 0x1F));
    SPI.transfer(value);
    csn_high();
    SPI.endTransaction();
}

uint8_t radio_read_reg(uint8_t reg)
{
    SPI.beginTransaction(kSpi);
    csn_low();
    SPI.transfer(CMD_R_REGISTER | (reg & 0x1F));
    uint8_t v = SPI.transfer(0xFF);
    csn_high();
    SPI.endTransaction();
    return v;
}

/* Addresses are given MSByte first; the radio wants the LSByte first. */
static void write_addr(uint8_t reg, const uint8_t addr[ADDR_LEN])
{
    SPI.beginTransaction(kSpi);
    csn_low();
    SPI.transfer(CMD_W_REGISTER | (reg & 0x1F));
    for (int8_t i = ADDR_LEN - 1; i >= 0; i--)
        SPI.transfer(addr[i]);
    csn_high();
    SPI.endTransaction();
}

static uint8_t rf_setup_value(void)
{
    uint8_t v = g_cfg.power & 0x07;
    switch (g_cfg.rate) {
    case RADIO_RATE_250K: v |= 0x20; break;   /* DR_LOW=1, DR_HIGH=0 */
    case RADIO_RATE_2M:   v |= 0x08; break;   /* DR_LOW=0, DR_HIGH=1 */
    default: break;                            /* 1 Mbps: 00 */
    }
    return v;
}

static const uint8_t kCfgBase = CFG_EN_CRC | CFG_CRCO;   /* CRC enabled, 2 bytes */

bool radio_begin(const RadioConfig &cfg)
{
    g_cfg = cfg;
    pinMode(g_cfg.pin_ce, OUTPUT);
    digitalWrite(g_cfg.pin_ce, LOW);
    pinMode(g_cfg.pin_csn, OUTPUT);
    csn_high();
    SPI.begin();

    delay(100);   /* datasheet: 100 ms power-on delay after VDD >= 1.9 V */

    /* Presence check: registers are writable in shutdown. Use two different patterns so a
     * floating MISO (reads 0x00 or 0xFF) cannot pass. */
    write_reg(REG_SETUP_AW, 0x03);
    write_reg(REG_RF_CH, 0x2A);
    if (radio_read_reg(REG_SETUP_AW) != 0x03 || radio_read_reg(REG_RF_CH) != 0x2A)
        return false;
    write_reg(REG_SETUP_AW, 0x02);
    write_reg(REG_RF_CH, 0x15);
    if (radio_read_reg(REG_SETUP_AW) != 0x02 || radio_read_reg(REG_RF_CH) != 0x15)
        return false;

    /* Write every register explicitly; never rely on reset values. */
    write_reg(REG_CONFIG, kCfgBase);                       /* shutdown */
    write_reg(REG_EN_AA, 0x03);                            /* auto-ACK on pipes 0 and 1 */
    write_reg(REG_EN_RXADDR, 0x02);
    write_reg(REG_SETUP_AW, 0x03);                         /* 5-byte addresses */
    write_reg(REG_SETUP_RETR, (uint8_t)(((g_cfg.retry_delay & 0x0F) << 4) | (g_cfg.retry_count & 0x0F)));
    write_reg(REG_RF_CH, g_cfg.channel & 0x7F);
    write_reg(REG_RF_SETUP, rf_setup_value());
    write_reg(REG_RX_PW_P0, PAYLOAD_LEN);
    write_reg(REG_RX_PW_P1, PAYLOAD_LEN);
    write_reg(REG_DYNPD, 0x00);                            /* static payload length */
    write_reg(REG_FEATURE, 0x00);
    spi_cmd(CMD_FLUSH_TX);
    spi_cmd(CMD_FLUSH_RX);
    write_reg(REG_STATUS, ST_RX_DR | ST_TX_DS | ST_MAX_RT);

    return radio_read_reg(REG_RF_CH) == (g_cfg.channel & 0x7F) &&
           radio_read_reg(REG_RF_SETUP) == rf_setup_value();
}

void radio_standby(void)
{
    digitalWrite(g_cfg.pin_ce, LOW);
}

void radio_wake(void)
{
    digitalWrite(g_cfg.pin_ce, LOW);
    write_reg(REG_CONFIG, (uint8_t)(kCfgBase | CFG_PWR_UP));
    delay(2);                                              /* crystal start 1.5-2 ms */
}

void radio_sleep(void)
{
    digitalWrite(g_cfg.pin_ce, LOW);
    write_reg(REG_CONFIG, kCfgBase);                       /* PWR_UP = 0 */
}

void radio_listen(const uint8_t addr[5])
{
    digitalWrite(g_cfg.pin_ce, LOW);
    write_reg(REG_CONFIG, (uint8_t)(kCfgBase | CFG_PWR_UP | CFG_PRIM_RX));
    write_reg(REG_EN_RXADDR, 0x02);                        /* pipe 1 only */
    write_addr(REG_RX_ADDR_P1, addr);
    write_reg(REG_RX_PW_P1, PAYLOAD_LEN);
    spi_cmd(CMD_FLUSH_RX);
    write_reg(REG_STATUS, ST_RX_DR | ST_TX_DS | ST_MAX_RT);
    digitalWrite(g_cfg.pin_ce, HIGH);
    delayMicroseconds(130);                                /* standby -> RX, 120-130 us */
}

bool radio_rx_available(void)
{
    return (radio_read_reg(REG_FIFO_STATUS) & 0x01) == 0;  /* RX_EMPTY == 0 */
}

bool radio_rx_read(uint8_t buf[8])
{
    if (!radio_rx_available())
        return false;
    SPI.beginTransaction(kSpi);
    csn_low();
    SPI.transfer(CMD_R_RX_PAYLOAD);
    for (uint8_t i = 0; i < PAYLOAD_LEN; i++)
        buf[i] = SPI.transfer(0xFF);
    csn_high();
    SPI.endTransaction();
    if (radio_read_reg(REG_FIFO_STATUS) & 0x01)            /* FIFO drained: clear RX_DR */
        write_reg(REG_STATUS, ST_RX_DR);
    return true;
}

void radio_rx_flush(void)
{
    spi_cmd(CMD_FLUSH_RX);
    write_reg(REG_STATUS, ST_RX_DR);
}

RadioTxResult radio_send(const uint8_t dest[5], const uint8_t payload[8])
{
    digitalWrite(g_cfg.pin_ce, LOW);                       /* PRIM_RX may only change in standby */
    write_reg(REG_CONFIG, (uint8_t)(kCfgBase | CFG_PWR_UP));   /* PTX */
    write_reg(REG_EN_RXADDR, 0x01);                        /* pipe 0 receives the hardware ACK */
    write_addr(REG_TX_ADDR, dest);
    write_addr(REG_RX_ADDR_P0, dest);
    spi_cmd(CMD_FLUSH_TX);
    write_reg(REG_STATUS, ST_RX_DR | ST_TX_DS | ST_MAX_RT);

    SPI.beginTransaction(kSpi);
    csn_low();
    SPI.transfer(CMD_W_TX_PAYLOAD);
    for (uint8_t i = 0; i < PAYLOAD_LEN; i++)
        SPI.transfer(payload[i]);
    csn_high();
    SPI.endTransaction();

    digitalWrite(g_cfg.pin_ce, HIGH);                      /* >= 10 us pulse starts TX */
    delayMicroseconds(20);
    digitalWrite(g_cfg.pin_ce, LOW);

    /* Worst case: (ARC+1) * (ARD + frame) with frame <= 1.2 ms at 250 kbps, plus 3 ms margin. */
    uint32_t budget_us = (uint32_t)(g_cfg.retry_count + 1) *
                         ((uint32_t)(g_cfg.retry_delay + 1) * 250UL + 1200UL) + 3000UL;
    uint32_t t0 = micros();
    RadioTxResult res = RADIO_TX_TIMEOUT;
    for (;;) {
        uint8_t st = spi_cmd(CMD_NOP);
        if (st & ST_TX_DS)  { res = RADIO_TX_OK;     break; }
        if (st & ST_MAX_RT) { res = RADIO_TX_NO_ACK; break; }
        if ((uint32_t)(micros() - t0) > budget_us)
            break;
    }
    write_reg(REG_STATUS, ST_TX_DS | ST_MAX_RT);
    if (res != RADIO_TX_OK)
        spi_cmd(CMD_FLUSH_TX);                             /* do not leave a stale frame queued */
    return res;
}

bool radio_rssi_above_minus60(void)
{
    return (radio_read_reg(REG_RSSI) & 0x01) != 0;
}
