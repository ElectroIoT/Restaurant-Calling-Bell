"""LCSC part data looked up on lcsc.com with the built-in browser on 2026-10-29.

Only values that were actually read from LCSC search results are filled in. price_usd is the unit price
at the quantity tier named in `tier` (USD, LCSC website, may change). `status`:
  ok        part found on LCSC with stock when checked
  verify    found but something needs checking before ordering (see note)
  external  not an LCSC item (user already owns it / bought elsewhere)
"""

PARTS = {
    "C14877":  dict(mpn="ATMEGA328P-AU", mfr="Microchip", desc="AVR 8-bit MCU 32KB TQFP-32", stock=16356, price_usd=2.3218, tier="1+", status="ok"),
    "C1711":   dict(mpn="CL21B104KBCNNNC", mfr="Samsung", desc="100nF 50V X7R 0805", stock=1693260, price_usd=0.0139, tier="20+", status="ok"),
    "C15850":  dict(mpn="CL21A106KAYNNNE", mfr="Samsung", desc="10uF 25V X5R 0805", stock=1698440, price_usd=0.0441, tier="20+", status="ok"),
    "C5674":   dict(mpn="CL21A226MQQNNNE", mfr="Samsung", desc="22uF 6.3V X5R 0805", stock=2820030, price_usd=0.0445, tier="10+", status="ok"),
    "C113825": dict(mpn="CC0805JRNPO9BN180", mfr="Yageo", desc="18pF 50V NP0 5% 0805", stock=81100, price_usd=0.0122, tier="50+", status="ok",
                    note="Samsung CL21C180JBANNNC (C170109) showed 0 stock"),
    "C1710":   dict(mpn="CL21B103KBANNNC", mfr="Samsung", desc="10nF 50V X7R 0805", stock=25550, price_usd=0.0123, tier="50+", status="ok"),
    "C17513":  dict(mpn="0805W8F1001T5E", mfr="UNI-ROYAL", desc="1k 1% 0805", stock=35589200, price_usd=0.0035, tier="100+", status="ok"),
    "C17709":  dict(mpn="0805W8F4703T5E", mfr="UNI-ROYAL", desc="470k 1% 0805", stock=124100, price_usd=0.0027, tier="100+", status="ok"),
    "C17414":  dict(mpn="0805W8F1002T5E", mfr="UNI-ROYAL", desc="10k 1% 0805", stock=41045900, price_usd=0.0034, tier="100+", status="ok"),
    "C27834":  dict(mpn="0805W8F5101T5E", mfr="UNI-ROYAL", desc="5.1k 1% 0805", stock=560700, price_usd=0.0055, tier="100+", status="ok"),
    "C17673":  dict(mpn="0805W8F4701T5E", mfr="UNI-ROYAL", desc="4.7k 1% 0805", stock=1255600, price_usd=0.0039, tier="100+", status="ok"),
    "C51118":  dict(mpn="AP2112K-3.3TRG1", mfr="Diodes Inc.", desc="3.3V 600mA LDO SOT-23-5", stock=2220, price_usd=0.1792, tier="5+", status="ok",
                    note="clone C23380830 (Tech Public) also listed; original preferred"),
    "C126818": dict(mpn="1206L110THYR", mfr="Littelfuse", desc="PTC resettable fuse 1.1A hold 1206", stock=0, price_usd=0.0286, tier="1+", status="verify",
                    note="listed under 'Other Suppliers' only (lead time 5-7 business days), not LCSC stock; confirm hold/trip rating and price"),
    "C193402": dict(mpn="SMF5.0A", mfr="MDD", desc="TVS 5V SOD-123FL", stock=496940, price_usd=0.0277, tier="20+", status="ok",
                    note="Littelfuse SMF5.0A is C151296 at $0.1312"),
    "C2905353": dict(mpn="XC32M4-8.000-F18LJDT", mfr="HCI", desc="8MHz crystal 3.2x2.5mm", stock=16285, price_usd=0.1906, tier="5+", status="verify",
                    note="load capacitance 18 pF inferred from the 'F18' suffix of the MPN; confirm on the datasheet"),
    "C81464":  dict(mpn="MMBT3904LT1G", mfr="onsemi", desc="NPN 40V 200mA SOT-23", stock=1576650, price_usd=0.0168, tier="50+", status="ok"),
    "C165948": dict(mpn="TYPE-C-31-M-12", mfr="Korean Hroparts Elec", desc="USB Type-C 16P receptacle", stock=280995, price_usd=0.1736, tier="5+", status="ok"),
    "C2295":   dict(mpn="KT-0805R", mfr="Hubei KENTO", desc="Red LED 0805", stock=87550, price_usd=0.0106, tier="50+", status="ok"),
    "C2297":   dict(mpn="KT-0805G", mfr="Hubei KENTO", desc="Green LED 0805", stock=1381250, price_usd=0.0166, tier="50+", status="ok"),
    "C318884": dict(mpn="TS-1187A-B-A-B", mfr="XKB Connection", desc="Tact switch 5.1x5.1mm SMD", stock=875440, price_usd=0.0208, tier="20+", status="ok"),
    "C964845": dict(mpn="MY-2450-01", mfr="MYOUNG", desc="CR2450 SMT battery holder", stock=5355, price_usd=0.1883, tier="5+", status="verify",
                    note="polarity of the two side pads is not stated in the drawing text: measure it, then fit JP1+JP2 or JP3+JP4"),
    "C474881": dict(mpn="KF301-5.0-2P", mfr="Cixi Kefa", desc="Screw terminal 2P 5.0mm", stock=53450, price_usd=0.1027, tier="5+", status="ok"),
    "C96093":  dict(mpn="TMB12A05", mfr="Huaneng", desc="Active buzzer 4-8V 12mm THT", stock=78275, price_usd=0.195, tier="5+", status="ok"),
    "C56153599": dict(mpn="PZ254V-12-06P-T", mfr="XFCN", desc="Pin header 2x3 2.54mm", stock=1000, price_usd=0.024, tier="20+", status="ok"),
    "C2897371": dict(mpn="PM254-1-08-Z-8.5", mfr="HCTL", desc="Female header 1x8 2.54mm", stock=10248, price_usd=0.1547, tier="6+", status="ok"),
    "C2897367": dict(mpn="PM254-1-04-Z-8.5", mfr="HCTL", desc="Female header 1x4 2.54mm", stock=9395, price_usd=0.1167, tier="5+", status="ok"),
    "C492401":  dict(mpn="PZ254V-11-02P", mfr="XFCN", desc="Pin header 1x2 2.54mm", stock=887950, price_usd=0.018, tier="50+", status="ok"),
    "C2887273": dict(mpn="RVT220UF16V67RV0015", mfr="KNSCHA", desc="220uF 16V aluminium electrolytic SMD", stock=215560, price_usd=0.0351, tier="20+", status="verify",
                     note="body size inferred from the '67' in the MPN (6.3x7.7?); confirm before layout is frozen"),
    "C17477":  dict(mpn="0805W8F0000T5E", mfr="UNI-ROYAL", desc="0 ohm jumper 0805", stock=2518900, price_usd=0.0045, tier="100+", status="ok"),
    "C304038": dict(mpn="1N4148W", mfr="PANJIT", desc="Switching diode SOD-123", stock=6343, price_usd=0.0087, tier="500+", status="verify",
                    note="listed under 'Other Suppliers' (lead time 3-5 business days), not LCSC stock; any SOD-123 1N4148W works"),
}
