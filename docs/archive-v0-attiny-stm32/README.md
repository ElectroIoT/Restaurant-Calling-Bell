# SI24R1 Calling Bell (restaurant tables)

Wireless table-call system. Button nodes (ATtiny85, coin cell) send a call over SI24R1; the receiver
(STM32C031F6P6) queues it, plays that table's tune, shows the table number and sends back an ACK.
Full design: `calling-bell-plan.html`.

## Layout

```
firmware/
  common/bell_protocol.h   shared packet structs, CRC-8, radio constants (used by every board)
  link_test/tx.cpp         step 1: Nano acting as a button node
  link_test/rx.cpp         step 1: Nano acting as the receiver
  platformio.ini           envs: link_tx, link_rx
```

## Step 1: radio link test (two Arduino Nanos, two SI24R1 modules)

Wiring, same on both boards:

| SI24R1 | Nano |
|---|---|
| VCC | external 3.3 V (not the Nano 5 V pin), 10 uF + 100 nF at the module |
| GND | GND (shared with the Nano) |
| CE | D9 |
| CSN | D10 |
| SCK | D13 |
| MOSI | D11 |
| MISO | D12 |

The Nano's 3.3 V pin is weak; a flaky link is almost always supply noise. Power the radio from a
separate 3.3 V regulator and keep the 10 uF capacitor right at the module.

Run:

```
cd firmware
pio run -e link_tx -t upload      # board 1, then open the monitor
pio run -e link_rx -t upload      # board 2
pio device monitor -b 115200
```

Pass criteria:
- TX prints `ACK` and `status 0` for every packet, with `ok N/N`.
- RX prints `table 5` with the same `seq` values and `badcrc 0`.
- Move the boards apart and through a wall; note where `NO ACK` starts.

Both sides print `RADIO NOT FOUND` if the module is unwired or under-powered.

## Next steps

2. Receiver on the STM32C031: SPI1 driver, same receive logic as `link_test/rx.cpp`.
3. ATtiny85 button: USI SPI, sleep, press types (short / long / double), ACK feedback LED.
4. PAM8403 audio and per-table tunes, then TM1637 display, call queue and "Served" button.
5. DFPlayer voice, ESP32 alerts to phone or RestoProERP.

Status: step 1 written, not yet compiled or run on hardware.
