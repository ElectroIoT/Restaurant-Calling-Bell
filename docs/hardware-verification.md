# What is verified and what is assumed

| Claim | Status | Evidence |
|---|---|---|
| Firmware compiles for ATmega328P (Uno and 3.3 V PCB targets, 22 builds) | **verified** | `python tools/build_firmware.py`, avr-gcc 7.3 via arduino-cli 1.5.1 |
| Protocol, call table, retry engine, EEPROM ring behave as specified in the model | **verified in software** | 476 host checks + 10-remote simulation (`tests/protocol-tests`) |
| SI24R1 register map, power encoding, timings, currents, pinout | **read from the two PDFs in `docs/datasheets`** | datasheet text |
| The driver works on a real SI24R1 module | **not tested** | no hardware attached to this PC |
| DFPlayer Mini commands, pin order, row spacing | **not verified** | datasheet not in the repo; from memory of the public command table |
| OLED SSD1306 init sequence | **not tested** | standard sequence, no display attached |
| CR2450 holder pad geometry and size | read from the vendor drawing (LCSC C964845); **polarity unknown** | `datasheets-lcsc` drawing, jumpers added |
| Tact switch land pattern | read from the vendor drawing (LCSC C318884) | |
| Crystal load capacitance 18 pF, bulk cap body size, KF301 pitch | **inferred from part numbers** | check before ordering |
| LCSC stock and prices | looked up 2026-10-29 with the browser; change daily | `hardware/tools/parts.py` |
| Battery life, range, reliability | **not measured; estimates only** | `docs/battery-life-calculation.md` |
| KiCad reference boards | ERC/DRC run in KiCad 10 (0 errors on the last runs, silkscreen warnings only); **auto-routed, no human review; not fabricated** | `hardware/*/kicad/*.rpt` |

Not claimed anywhere: production readiness, regulatory compliance (2.4 GHz equipment must meet your country's rules; the default channel
2476 MHz is inside the 2400-2483.5 MHz band), or a tested range.

## Risks to close before fabrication

1. Radio driver on real modules (tests 2-9).
2. DFPlayer pin order and row spacing against the actual module.
3. CR2450 sag test, or switch to 2 x AAA (test 12).
4. Antenna placement: module edge overhang and clearance from the holder and case.
5. Sleep current measurement (test 13).
6. PTC (1206L110) and 1N4148W are not LCSC stock items in the lookup: pick substitutes if needed.
