# Assembly guide

## Order of work

1. **Prototype first.** Two Uno boards, two radio modules, separate 3.3 V supply ([wiring](../hardware/diagrams/wiring.md)). Run tests 2-9 of the
   [test plan](test-plan.md) before ordering any PCB.
2. Measure the real parts you will place on the PCB: SI24R1 module pad pitch and length, DFPlayer row spacing and pin order, OLED pin order,
   the coin-cell holder's pad polarity. Update the footprints ([EasyEDA guide](easyeda-guide.md)).
3. Order the boards and the LCSC parts ([BOM summary](../hardware/bom/BOM-SUMMARY.md); check the "needs checking" table first).

## Remote

1. Solder the SMD parts (0805 hot-air or iron), then the module: 8 half-holes onto the 8 pads, antenna end **beyond the board edge**.
2. Measure the coin-cell holder polarity, then fit **JP1+JP2 or JP3+JP4** (two of the four 0 ohm parts, not all four).
3. Flash the fuses and the firmware with `TABLE_ID` through the ISP header **before** closing the case; write the id on the case.
4. Power: fit the CR2450 **or** wire 2xAAA to J4, never both.
5. Quick check: LED flashes while waiting, solid for 1 s when the receiver accepted.

## Receiver

1. Solder SMD parts, USB-C, headers; plug the DFPlayer Mini, OLED and speaker last.
2. Check 5 V and 3.3 V on TP1/TP2 before fitting modules.
3. Prepare the microSD card ([audio guide](audio-guide.md)), flash with `-DBELL_BOARD_PCB=1`.
4. Mount the OLED behind the front window, buttons accessible, antenna end clear of metal, speaker in a small ported box.

## Enclosure assumptions

Reference remote board 44 x 64 mm (the coin holder is 36.8 x 23.2 mm and sits on the bottom); reference receiver board 100 x 72 mm.
Plastic enclosures are fine; metal near the antenna is not.
