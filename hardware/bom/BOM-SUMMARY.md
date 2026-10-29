# BOM summary (LCSC prices read 2026-10-29)

Only parts that were looked up on lcsc.com appear with a price. Modules (SI24R1, DFPlayer Mini, OLED), the speaker,
coin cell, microSD card and enclosure are NOT priced: they were either owned already or not looked up.

| Board | LCSC parts, essential (USD) | LCSC parts incl. optional (USD) | Lines |
|---|---|---|---|
| Remote (1 pc) | 2.76 | 2.80 | 15 |
| Remote (10 pcs) | 27.64 | 28.00 | 15 |
| Receiver (1 pc) | 3.95 | 4.15 | 28 |
| **System: 1 receiver + 10 remotes** | **31.58** | **32.15** | |

Unit prices use the quantity tier shown in the CSV (often a 5+/20+/100+ tier, so 1-piece prices are higher).
Not included: PCB fabrication, shipping, import duty/tax, modules, battery, enclosure.

## Parts that need checking before ordering

| LCSC | MPN | Why |
|---|---|---|
| C126818 | 1206L110THYR | listed under 'Other Suppliers' only (lead time 5-7 business days), not LCSC stock; confirm hold/trip rating and price |
| C2887273 | RVT220UF16V67RV0015 | body size inferred from the '67' in the MPN (6.3x7.7?); confirm before layout is frozen |
| C2905353 | XC32M4-8.000-F18LJDT | load capacitance 18 pF inferred from the 'F18' suffix of the MPN; confirm on the datasheet |
| C304038 | 1N4148W | listed under 'Other Suppliers' (lead time 3-5 business days), not LCSC stock; any SOD-123 1N4148W works |
| C964845 | MY-2450-01 | polarity of the two side pads is not stated in the drawing text: measure it, then fit JP1+JP2 or JP3+JP4 |
