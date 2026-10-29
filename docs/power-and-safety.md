# Power and safety checks

Numbers marked (DS) come from `docs/datasheets/`. Everything else is an engineering estimate. **Nothing here has been measured.**

## Limits

| Item | Limit | Where used |
|---|---|---|
| SI24R1 supply | 1.9-3.6 V (DS); module sheet recommends 3.0-3.3 V | both boards |
| SI24R1 I/O | tolerant to 5.25 V (DS); module sheet suggests 4.7 k series resistors to a 5 V MCU | Uno prototype only |
| ATmega328P at 4 MHz | 1.8-5.5 V (ATmega328P datasheet, not in repo) | remote |
| ATmega328P at 8 MHz | 2.7-5.5 V; the receiver runs at 3.3 V | receiver |
| TX current | 12 mA at 0 dBm, 16 mA at 4 dBm, 25 mA at 7 dBm (DS) | remote battery |
| RX current | 14 mA at 250 kbps (DS) | receiver radio |
| Fresh 2xAAA alkaline | about 3.2 V, under the 3.6 V maximum. **Do not use fresh lithium AAA (about 3.6 V).** | remote option |

## Remote battery options

| | CR2450 | CR2032 | 2 x AAA alkaline |
|---|---|---|---|
| Rated capacity (typical, vendor) | about 620 mAh | about 225 mAh | about 1000 mAh (low drain) |
| Internal resistance | tens of ohms, rising to 100+ near end of life | similar, higher | below 1 ohm per cell |
| 12 mA TX pulse sag (estimate) | 0.2-0.5 V fresh; can reach the 1.9 V limit near end of life | worse | negligible |
| Verdict | **acceptable only after the sag test passes** (test 12) | not recommended | **recommended if reliability matters** |

Run `python tools/battery_calc.py` for the estimate and its assumptions. The PCB takes either source on the same rail
(coin holder pads or the 2-pin AAA header, **never both at once**).

* Local capacitance at the radio: 100 nF + 22 uF (0805). A ceramic 22 uF 6.3 V X5R loses roughly half its capacitance at 3 V bias,
  so treat it as about 10 uF. Ceramic is chosen on purpose: its leakage is far below the 2-3 uA sleep budget; do not substitute
  electrolytic or tantalum on the remote.
* Brown-out: use the 1.8 V fuse setting (`efuse 0xFE`); the BOD is also switched off in software during sleep. Below roughly 1.9 V the
  radio may misbehave: that is what the low-battery flag (below 2.4 V, `LOW_BATT_MV`) is for.
* Supply start-up: the radio needs 100 ms after VDD reaches 1.9 V (DS); `radio_begin()` waits.
* Reverse polarity: the coin holder is keyed; the AAA header is not. Mark it and use a keyed connector if staff will change cells.
  There is **no charger** on the remote. Never connect a charger to a primary (non-rechargeable) cell.
* The holder pad polarity of the chosen CR2450 holder is not stated in its drawing text: the PCB has four 0 ohm jumpers so you can
  choose after measuring the part with a cell and a multimeter.

## Receiver

* 5 V in via USB-C (power only, 5.1 k on CC1/CC2), 1.1 A PTC and a 5 V TVS diode. The PTC part number needs checking (see BOM).
* 3.3 V from AP2112K-3.3 (600 mA, 55 uA quiescent). Load: MCU about 5 mA, radio RX 15 mA, OLED 10-20 mA: well inside the rating.
  Dissipation at 5 V in, 50 mA: (5-3.3) x 0.05 = 85 mW, fine in SOT-23-5.
* DFPlayer Mini runs from 5 V. Its built-in amplifier is commonly quoted at 3 W; into 4 ohm at 5 V the real output is lower and the
  speaker must be rated at least as high as the amplifier can deliver. Start with `AUDIO_VOLUME 20`, check amplifier temperature and
  supply droop (the 220 uF + 2 x 10 uF at the 5 V rail are there for speaker peaks), and use a 5 V 2 A supply.
* The DFPlayer RX pin gets a 1 k series resistor; its TX goes straight to the 3.3 V MCU RX.
* Mains is never on the board: use a certified 5 V USB charger.
* Thermal: no part dissipates more than about 0.1 W except the DFPlayer amplifier at high volume (touch-test it during the soak test).
