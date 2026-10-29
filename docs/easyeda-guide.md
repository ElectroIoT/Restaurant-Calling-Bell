# EasyEDA Pro layout guide

You lay the boards out yourself in EasyEDA Pro (web). The KiCad boards in `hardware/*/kicad` are only a **reference** (they passed
ERC and DRC in KiCad 10, but were auto-routed and have not been reviewed by a person). Use these files as the single source of truth for
connectivity and parts:

| File | Use |
|---|---|
| `hardware/<board>/easyeda/netlist.csv` | every net and the `REF.PIN` it connects |
| `hardware/<board>/easyeda/nets-by-pin.csv` | the same, per component pin (use it to check your schematic) |
| `hardware/<board>/easyeda/bom-easyeda.csv` | BOM with LCSC numbers in the Supplier Part column (EasyEDA import format) |
| `hardware/<board>/easyeda/placement-hint.csv` | the reference placement (mm, top-left origin) |
| `hardware/<board>/kicad/*-schematic.pdf`, `pcb-3d-top.png` | pictures of the reference design |
| `hardware/<board>/bom/*.csv` | full BOMs with price, stock, status, substitutes |

Regenerate after changing `hardware/tools/*_design.py`: `python hardware/tools/gen_easyeda_pack.py` and `python hardware/tools/gen_bom.py`.

## Parts that need a footprint check in EasyEDA

Use the LCSC part's own EasyEDA footprint wherever the part is in the LCSC library (search the C number). Compare it with the numbers below.

| Part | LCSC | What to check |
|---|---|---|
| SI24R1 half-hole module (12 x 18 mm) | n/a (user's module) | 8 pads at **1.27 mm pitch**, order from the antenna-up view: `3V3, GND, CE, CSN, SCK, MOSI, MISO, IRQ`. Overall 12 x 18 mm; component area 11.5 mm, antenna the remaining 6.5 mm. Measure **your** module with calipers. |
| CR2450 holder MY-2450-01 | C964845 | 36.8 x 23.2 mm. Two side pads 6.15 x 6.0 mm, 8.0 mm from the bottom edge, 9.2 mm from the top edge (vendor drawing). **Which pad is + is not stated**: measure it, or fit the 0 ohm jumpers JP1+JP2 / JP3+JP4. |
| Tact switch TS-1187A | C318884 | 5.1 x 5.1 mm, pads centres 6.0 mm apart in X and 3.75 mm in Y, pads 1.0 x 0.75 mm; the two top pads are one contact, the two bottom pads the other. |
| USB-C 16P TYPE-C-31-M-12 | C165948 | mouth faces the board edge; signal pads at the rear. Verify against the LCSC footprint preview. |
| DFPlayer Mini sockets | n/a (Robocraze MP3-TF-16P, listing: 21 x 21 x 10 mm, 3.2-5 V; a buyer review notes the board has no printed pin labels) | two 1x8 2.54 mm rows. Row spacing **17.78 mm is an assumption**. Print the footprint 1:1 on paper and hold the real module over it before ordering. Pin order assumed: left column 1 VCC, 2 RX, 3 TX, 4 DAC_R, 5 DAC_L, 6 SPK2, 7 GND, 8 SPK1; right column 9 IO1, 10 GND, 11 IO2, 12 ADKEY1, 13 ADKEY2, 14 USB+, 15 USB-, 16 BUSY. |
| OLED header | C2897367 | modules come with two pin orders (GND VCC SCL SDA or VCC GND SDA SCL). Check yours and re-order the 4 net names if needed. |
| 220 uF bulk cap | C2887273 | body size unconfirmed (6.3 x 7.7 mm assumed) |
| Screw terminal KF301-5.0-2P | C474881 | 5.0 mm pitch assumed |

## Layout rules

**Both boards**
* 2 layers, 1.6 mm. Solid GND pour on both sides, stitched with vias; no GND split.
* 0805 passives everywhere (as requested); no component smaller than 0805 on the BOM.
* Decoupling 100 nF within 3 mm of each MCU VCC pin and the radio 3V3 pad; the 22 uF at the radio pads.
* AREF 100 nF to GND, AVCC to the supply through its own 100 nF.

**SI24R1 module (most important)**
* Mount the module at the **board edge**, with the antenna end (the 6.5 mm above the 11.5 mm component area) hanging **beyond the edge**
  of the PCB, or over a board cut-out. If the antenna must sit over FR4, keep **no copper, no parts, no traces on every layer** under and
  1-2 mm around the antenna area (the module sheet says the same).
* Also keep both layers free of copper under the module body except the 8 pads; the module underside has exposed vias.
* No coin-cell holder, battery, metal enclosure part, speaker or wiring within about 10 mm of the antenna. Metal near the antenna detunes it
  (module sheet: use an SMA antenna if metal is unavoidable).
* Radio supply: short, wide trace from the 22 uF/100 nF pair; the TX pulses are 12-16 mA at 250 kbps.

**Remote**
* CR2450 holder on the **bottom** side, far from the antenna end; keep through-hole parts out of the holder footprint (their tails would
  hit the holder).
* The CALL button is a 5.1 mm tact on the PCB; for a big panel button use the 2-pin `EXT BUTTON` header in parallel.
* R2 (470 k) is an **external pull-up on purpose**: a stuck button then draws about 6 uA instead of about 100 uA (internal pull-up).
* Keep the ISP header (6 pins, 2.54 mm: MISO, VCC, SCK, MOSI, RESET, GND in the standard 2x3 order) reachable inside the enclosure.
* Enclosure assumption: reference board 44 x 64 mm, four M2.5 holes 3.2 mm from the corners. Adjust to your case.

**Receiver**
* USB-C at the bottom edge, its PTC and TVS right behind it; 220 uF and 2 x 10 uF close to the DFPlayer power pins.
* Keep the speaker terminal and the DFPlayer amplifier traces away from the radio and the crystal.
* Crystal: 3.2 x 2.5 mm, two 18 pF to GND, short traces, no signals underneath.
* OLED connector and the three buttons toward the front panel side. Test points: 5 V, 3V3, GND, MCU TX.

## Review checklist before sending to a fab

* Net names match `netlist.csv` (run EasyEDA's netlist compare against it).
* Every footprint in the table above checked against the real part or its datasheet drawing.
* Antenna keep-out verified on both layers, module overhanging the edge.
* ERC and DRC clean in EasyEDA; manual look at all USB-C, crystal and radio traces.
* 3D view check: the holder and DFPlayer do not collide with other parts or the case.
