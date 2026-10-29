# Radio driver choice

**Decision: a small register-level driver written against the Si24R1 datasheet**
([`radio_si24r1.cpp`](../firmware/common/src/radio_si24r1.cpp), 250 lines) behind the
[`radio_driver.h`](../firmware/common/src/radio_driver.h) API. Application code never includes a register map or a radio library.

## Why not RF24 or libsi24

| | RF24 (TMRh20) | libsi24 (robinkrens) | Own driver |
|---|---|---|---|
| Version checked | 1.6.2 (`library.properties` on GitHub, 2026-10-29) | README only (see below) | - |
| Si24R1 fit | Targets nRF24L01(+). **`RF_PWR` encoding differs**: Si24R1 uses a 3-bit field `RF_SETUP[2:0]` (7/4/3/1/0/-4/-6/-12 dBm); nRF24 uses 2 bits at `[2:1]`, so RF24's `setPALevel()` writes the wrong power. Detection helpers (`isChipConnected`, `begin`) are built for nRF24 registers. | Written for the Si24R1 but a HAL-glue style file; README lists receive, transmit, dynamic payloads, **"Multiple channels" not supported** | Exactly the registers used here, every one written explicitly |
| Features needed | Would work for data, but ACK-payload / DPL paths are not needed by this protocol | No auto-ACK pipe handling documented | static payload, auto-ACK on P0/P1, shutdown/standby control |
| Flash cost | several KB | small | about 1 KB |
| Licence | GPL-2.0 (affects redistribution of firmware images) | file in repo, not reviewed here | MIT (this repo) |

Not verified on hardware: whether every Si24R1 module behaves identically to the datasheet. The driver therefore (1) does a
two-pattern write/read-back presence check in `radio_begin()` and returns `false` (the firmware shows `RF FAIL` / blinks
a fatal pattern, never fails silently), (2) never relies on reset values, (3) re-initialises the receiver radio if the
channel register ever reads back wrong (brown-out recovery, checked every 5 s).

## Datasheet facts the driver depends on

| Fact | Source |
|---|---|
| 126 channels, 1 MHz steps, f = 2400 + RF_CH MHz | datasheet section 1, register 05 |
| Rates 250 kbps / 1 / 2 Mbps; `[RF_DR_LOW, RF_DR_HIGH]` = 10 / 00 / 01 | register 06 |
| Power codes 111=7 dBm ... 000=-12 dBm | register 06 |
| `W_REGISTER` only in shutdown, standby or idle-TX | SPI command table |
| Shutdown -> standby 1.5-2 ms, standby -> TX/RX 120-130 us; CE pulse >= 10 us | section 3.1 |
| TX current 12 mA at 0 dBm, RX 14 mA at 250 kbps, standby 15 uA, shutdown 1 uA | section 7.2 |
| Supply 1.9-3.6 V, I/O tolerant to 5.25 V | section 7 |
| RX FIFO holds 3 packets | section 3.1.5 |
| Registers and FIFOs are retained in shutdown | section 3.1.1 |

The module specification PDF (`SI24R1-M-module-specification.pdf`) gives the module pinout (3V3, GND, CE, CSN, SCK, MOSI, MISO, IRQ)
and its own supply note (3.0-3.3 V, 3.3 V recommended), antenna advice (no copper or parts under/around the PCB antenna) and
5 V MCU advice (4.7 k series resistors).

## Swapping the driver

Provide `radio_begin, radio_wake, radio_sleep, radio_standby, radio_listen, radio_rx_available, radio_rx_read,
radio_rx_flush, radio_send, radio_read_reg, radio_rssi_above_minus60` from another file (for example RF24 behind this API)
and exclude `radio_si24r1.cpp`. The application and the host tests do not change.
