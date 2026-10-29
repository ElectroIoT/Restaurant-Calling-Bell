# Pin maps and wiring

ATmega328P pin numbers are TQFP-32 package pins; "Dx/Ax" are Arduino pin names used in the firmware. SI24R1 module order
(antenna up, left to right): `1 3V3, 2 GND, 3 CE, 4 CSN, 5 SCK, 6 MOSI, 7 MISO, 8 IRQ`.

## Remote (Uno prototype pins equal the custom PCB pins)

| Signal | Arduino | ATmega328P pin | Goes to |
|---|---|---|---|
| SCK | D13 | PB5 (17) | radio SCK |
| MISO | D12 | PB4 (16) | radio MISO |
| MOSI | D11 | PB3 (15) | radio MOSI |
| CSN | D10 | PB2 (14) | radio CSN |
| CE | D9 | PB1 (13) | radio CE |
| IRQ | D2 | PD2 (32) | radio IRQ (wired, unused: STATUS is polled) |
| CALL button | D4 | PD4 (2) | button to GND, 470 k pull-up (PCB); PCINT20 wakes from power-down |
| Status LED | D5 | PD5 (9) | 1 k then LED to GND |
| RESET | - | PC6 (29) | 10 k to supply, ISP pin 5 |

Uno prototype: power the radio from a **separate 3.3 V source** (not the Uno 3.3 V pin), 22 uF + 100 nF at the module, common GND,
4.7 k series resistors in SCK, MOSI, CSN and CE (5 V MCU outputs into the radio). MISO is 3.3 V into the 5 V MCU: acceptable.
Use the internal pull-up (`BUTTON_EXTERNAL_PULLUP 0`, default for the Uno build).

### Connectors

| Ref | Pins | Purpose |
|---|---|---|
| J2 | 2x3, 2.54 mm | ISP: 1 MISO, 2 VCC, 3 SCK, 4 MOSI, 5 RESET, 6 GND |
| J3 | 1x2 | external big button (1 = signal, 2 = GND), in parallel with SW1 |
| J4 | 1x2 | 2xAAA input (1 = +, 2 = GND). **Never fit together with a coin cell.** |
| BT1 + JP1..JP4 | holder | polarity select: fit JP1+JP2 **or** JP3+JP4 |

## Receiver

| Signal | Arduino (Uno proto) | PCB MCU pin | Goes to |
|---|---|---|---|
| SCK / MISO / MOSI | D13 / D12 / D11 | 17 / 16 / 15 | radio |
| CSN / CE / IRQ | D10 / D9 / D2 | 14 / 13 / 32 | radio (IRQ unused) |
| OLED SDA / SCL | A4 / A5 | 27 / 28 | OLED module, 4.7 k pull-ups to 3V3 |
| Buzzer | D3 | PD3 (1) | 1 k to NPN base; active buzzer to 5 V; flyback diode |
| ACK / CLEAR / MUTE | D4 / D5 / D6 | 2 / 9 / 10 | buttons to GND (internal pull-ups) |
| DFPlayer TX -> MCU RX | D7 (SoftwareSerial) | **D0 (30) hardware UART** | DFPlayer TX |
| MCU TX -> DFPlayer RX | D8 (SoftwareSerial) | **D1 (31) hardware UART** | 1 k series, DFPlayer RX |
| DFPlayer BUSY | A0 | PC0 (23) | DFPlayer BUSY (low while playing) |
| RESET | - | 29 | 10 k pull-up, SW1, ISP |
| Crystal | - | 7 / 8 | 8 MHz, 2 x 18 pF |

The PCB uses the hardware UART for the DFPlayer because the 8 MHz crystal and 9600 baud are accurate there, and it is programmed
through ISP (no serial bootloader, so D0/D1 are free). On the Uno, D0/D1 belong to USB serial, hence SoftwareSerial on D7/D8.

Uno prototype power: radio and OLED from a separate 3.3 V regulator; DFPlayer from 5 V; common ground. The DFPlayer RX line gets
the 1 k resistor; DFPlayer TX (3.3 V) into the Uno RX pin is fine.

### Connectors

| Ref | Purpose |
|---|---|
| J1 | USB-C, power only (5.1 k on CC1 and CC2), PTC and TVS behind it |
| J3 | ISP 2x3 |
| J4 | OLED 1x4: `GND, 3V3, SCL, SDA` (check your module) |
| J6 / J7 | DFPlayer Mini sockets, 2 x 1x8, assumed 17.78 mm apart |
| J8 | speaker, 2-pin screw terminal |
| SW1 | reset; SW2 ACK; SW3 CLEAR (hold 2 s = clear all); SW4 MUTE |
| TP1..TP4 | 5 V, 3V3, GND, MCU TX |
