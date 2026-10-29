"""Single source of truth for the TABLE REMOTE board: parts, footprints, nets and placement.

gen_schematic.py, gen_pcb.py and gen_bom.py all read this file, so schematic, PCB and BOM cannot disagree.
Net 'NC' marks a pin as intentionally unconnected; pins not listed are also marked no-connect.
Power net "+3V0" is the battery rail (CR2450 or 2xAAA, 1.9 .. 3.3 V).
"""
PROJECT = "BellRemote"
TITLE = "Restaurant Calling Bell - Table Remote (ATmega328P + SI24R1)"
OUT_DIR = "remote/kicad"
COMMENT = "CR2450 or 2xAAA direct supply, no regulator, 4 MHz ATmega328P"

R0805 = "Resistor_SMD:R_0805_2012Metric"
C0805 = "Capacitor_SMD:C_0805_2012Metric"
HDR = "Connector_PinHeader_2.54mm:PinHeader_1x%02d_P2.54mm_Vertical"

S = {}


def comp(section, ref, lib, sym, value, fp, nets, **extra):
    d = dict(ref=ref, lib=lib, sym=sym, value=value, fp=fp, nets=nets)
    d.update(extra)
    S.setdefault(section, []).append(d)


def R(sec, ref, val, a, b, lcsc, **kw):
    comp(sec, ref, "Device", "R", val, R0805, {"1": a, "2": b}, lcsc=lcsc, **kw)


def C(sec, ref, val, a, b, lcsc, **kw):
    comp(sec, ref, "Device", "C", val, C0805, {"1": a, "2": b}, lcsc=lcsc, **kw)


# ------------------------------------------------------------------ 1. battery
sec = "1  BATTERY (fit ONE: CR2450 holder or 2xAAA wires)"
comp(sec, "BT1", "Device", "Battery_Cell", "CR2450", "BellParts:CR2450_Holder_MY-2450-01",
     {"1": "BT_A", "2": "BT_B"}, lcsc="C964845", note="polarity set by JP1..JP4")
# polarity select: populate JP1+JP2 if pad 1 is the + terminal, otherwise JP3+JP4
R(sec, "JP1", "0R", "BT_A", "+3V0", "C17477", dnp_note="fit JP1+JP2 OR JP3+JP4")
R(sec, "JP2", "0R", "BT_B", "GND", "C17477", dnp_note="fit JP1+JP2 OR JP3+JP4")
R(sec, "JP3", "0R", "BT_A", "GND", "C17477", dnp_note="fit JP3+JP4 OR JP1+JP2")
R(sec, "JP4", "0R", "BT_B", "+3V0", "C17477", dnp_note="fit JP3+JP4 OR JP1+JP2")
comp(sec, "J4", "Connector_Generic", "Conn_01x02", "2xAAA IN (+ / -)", HDR % 2,
     {"1": "+3V0", "2": "GND"}, lcsc="C492401", optional=True)
comp(sec, "#FLG01", "power", "PWR_FLAG", "PWR_FLAG", "", {"1": "+3V0"})
comp(sec, "#FLG02", "power", "PWR_FLAG", "PWR_FLAG", "", {"1": "GND"})
C(sec, "C5", "10uF", "+3V0", "GND", "C15850", note="bulk, 25V X5R: low leakage ceramic, never electrolytic/tantalum")

# ------------------------------------------------------------------ 2. MCU
sec = "2  ATMEGA328P-AU (4 MHz internal RC, CKDIV8 fuse + runtime /2)"
comp(sec, "U1", "MCU_Microchip_ATmega", "ATmega328P-A", "ATmega328P-AU",
     "Package_QFP:TQFP-32_7x7mm_P0.8mm",
     {"4": "+3V0", "6": "+3V0", "18": "+3V0", "3": "GND", "5": "GND", "21": "GND", "20": "AREF",
      "29": "RST", "13": "CE", "14": "CSN", "15": "MOSI", "16": "MISO", "17": "SCK", "32": "IRQ",
      "2": "BTN", "9": "LED_D"}, lcsc="C14877")
C(sec, "C1", "100nF", "+3V0", "GND", "C1711")
C(sec, "C2", "100nF", "+3V0", "GND", "C1711")
C(sec, "C3", "100nF", "+3V0", "GND", "C1711", note="AVCC")
C(sec, "C4", "100nF", "AREF", "GND", "C1711")
R(sec, "R1", "10k", "+3V0", "RST", "C17414", note="reset pull-up")
comp(sec, "J2", "Connector_Generic", "Conn_02x03_Odd_Even", "ISP",
     "Connector_PinHeader_2.54mm:PinHeader_2x03_P2.54mm_Vertical",
     {"1": "MISO", "2": "+3V0", "3": "SCK", "4": "MOSI", "5": "RST", "6": "GND"}, lcsc="C56153599")

# ------------------------------------------------------------------ 3. radio
sec = "3  SI24R1 RADIO MODULE (HC-24L01-M style, 8 half-holes)"
comp(sec, "U2", "Connector_Generic", "Conn_01x08", "SI24R1 module", "BellParts:SI24R1_Module_12x18",
     {"1": "+3V0", "2": "GND", "3": "CE", "4": "CSN", "5": "SCK", "6": "MOSI", "7": "MISO", "8": "IRQ"},
     external=True, note="user-supplied module (not on this BOM)")
C(sec, "C6", "100nF", "+3V0", "GND", "C1711", note="at the radio pins")
C(sec, "C7", "22uF", "+3V0", "GND", "C5674", note="TX current pulses (12-16 mA); 6.3V X5R, derates at 3V")

# ------------------------------------------------------------------ 4. user interface
sec = "4  CALL BUTTON + STATUS LED"
comp(sec, "SW1", "Switch", "SW_Push", "CALL", "BellParts:SW_TS-1187A", {"1": "BTN", "2": "GND"}, lcsc="C318884")
R(sec, "R2", "470k", "+3V0", "BTN", "C17709", note="external pull-up: a stuck button draws ~6 uA, internal pull-up would draw ~100 uA")
C(sec, "C8", "10nF", "BTN", "GND", "C1710")
comp(sec, "J3", "Connector_Generic", "Conn_01x02", "EXT BUTTON", HDR % 2,
     {"1": "BTN", "2": "GND"}, lcsc="C492401", optional=True, note="wire to a big panel/arcade button")
comp(sec, "D1", "Device", "LED", "red", "LED_SMD:LED_0805_2012Metric", {"1": "GND", "2": "LED_A"}, lcsc="C2295")
R(sec, "R3", "1k", "LED_D", "LED_A", "C17513")

# ------------------------------------------------------------------ 5. test points + mounting
sec = "5  TEST POINTS + MOUNTING"
for ref, net in [("TP1", "+3V0"), ("TP2", "GND"), ("TP3", "CE")]:
    comp(sec, ref, "Connector", "TestPoint", net, "TestPoint:TestPoint_Pad_D2.0mm", {"1": net})
for i in range(4):
    comp(sec, "H%d" % (i + 1), "Mechanical", "MountingHole", "M2.5", "MountingHole:MountingHole_2.7mm_M2.5", {})

# ================================================================== PCB data
BOARD = (44.0, 64.0)                       # mm, top-left origin; antenna end = top edge (y = 0)
# ref -> (x, y, rotation_deg [, "B" for bottom side])
P = {
    "U2": (22.0, 11.5, 0),                 # module pad row at y=11.5; body 11.5 mm on board, antenna 6.5 mm beyond the edge
    "C6": (13.0, 16.0, 90), "C7": (31.0, 16.0, 90),
    "U1": (22.0, 27.0, 0),
    "C1": (13.0, 23.0, 90), "C2": (13.0, 31.0, 90), "C3": (31.0, 31.0, 90), "C4": (31.0, 24.0, 90),
    "R1": (26.5, 19.2, 0),
    "C5": (13.0, 27.0, 90),
    "J2": (36.5, 14.0, 0), "J3": (5.5, 12.0, 0), "J4": (5.5, 19.5, 0),
    "SW1": (22.0, 40.0, 0), "R2": (28.5, 37.0, 0), "C8": (15.5, 37.0, 0),
    "D1": (31.0, 42.5, 0), "R3": (31.0, 46.0, 0),
    "BT1": (22.0, 46.5, 0, "B"),
    "JP1": (6.0, 37.0, 90), "JP2": (9.0, 37.0, 90), "JP3": (35.0, 37.0, 90), "JP4": (38.0, 37.0, 90),
    "TP1": (4.5, 26.0, 0), "TP2": (4.5, 29.4, 0), "TP3": (4.5, 32.8, 0),
    "H1": (3.2, 3.2, 0), "H2": (40.8, 3.2, 0), "H3": (3.2, 61.0, 0), "H4": (40.8, 61.0, 0),
}
PADMAP = {}
# rectangles (x0, y0, x1, y1) with no copper of any kind (antenna and module underside)
KEEPOUTS = [(14.5, 0.0, 29.5, 10.2)]
TEXTS = [
    ("CALL", 22.0, 35.2, 1.0, "F.SilkS", 0),
    ("ANTENNA - KEEP CLEAR", 22.0, 5.0, 0.8, "F.SilkS", 0),
    ("ISP", 36.5, 12.0, 0.8, "F.SilkS", 0),
    ("EXT BTN", 12.0, 12.8, 0.8, "F.SilkS", 0),
    ("AAA IN +/-", 12.5, 19.0, 0.8, "F.SilkS", 0),
    ("JP1+JP2 OR JP3+JP4 (BATT POLARITY)", 22.0, 33.5, 0.8, "F.SilkS", 0),
    ("Bell Remote v1", 22.0, 52.5, 1.0, "F.SilkS", 0),
    ("TABLE ID = firmware", 22.0, 54.5, 0.8, "F.SilkS", 0),
    ("github.com/ElectroIoT/Restaurant-Calling-Bell", 22.0, 30.0, 0.8, "B.SilkS", 0),
]
