"""Single source of truth for the COUNTER RECEIVER board: parts, footprints, nets and placement."""
PROJECT = "BellReceiver"
TITLE = "Restaurant Calling Bell - Counter Receiver (ATmega328P + SI24R1 + OLED + DFPlayer)"
OUT_DIR = "receiver/kicad"
COMMENT = "5V USB-C in, 3.3V LDO for MCU/radio/OLED, DFPlayer Mini on 5V"

R0805 = "Resistor_SMD:R_0805_2012Metric"
C0805 = "Capacitor_SMD:C_0805_2012Metric"
HDR = "Connector_PinHeader_2.54mm:PinHeader_1x%02d_P2.54mm_Vertical"
SKT = "Connector_PinSocket_2.54mm:PinSocket_1x%02d_P2.54mm_Vertical"
SW_FP = "BellParts:SW_TS-1187A"

S = {}


def comp(section, ref, lib, sym, value, fp, nets, **extra):
    d = dict(ref=ref, lib=lib, sym=sym, value=value, fp=fp, nets=nets)
    d.update(extra)
    S.setdefault(section, []).append(d)


def R(sec, ref, val, a, b, lcsc, **kw):
    comp(sec, ref, "Device", "R", val, R0805, {"1": a, "2": b}, lcsc=lcsc, **kw)


def C(sec, ref, val, a, b, lcsc, **kw):
    comp(sec, ref, "Device", "C", val, C0805, {"1": a, "2": b}, lcsc=lcsc, **kw)


# ------------------------------------------------------------------ 1. 5 V input
sec = "1  USB-C 5 V INPUT (power only) + PROTECTION"
comp(sec, "J1", "Connector", "USB_C_Receptacle_USB2.0_16P", "USB-C 5V",
     "Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12",
     {"A1": "GND", "A12": "GND", "B1": "GND", "B12": "GND", "SH": "GND",
      "A4": "VBUS", "A9": "VBUS", "B4": "VBUS", "B9": "VBUS", "A5": "CC1", "B5": "CC2",
      "A6": "NC", "B6": "NC", "A7": "NC", "B7": "NC", "A8": "NC", "B8": "NC"}, lcsc="C165948")
comp(sec, "F1", "Device", "Polyfuse", "1.1A PTC", "Fuse:Fuse_1206_3216Metric", {"1": "VBUS", "2": "+5V"}, lcsc="C126818")
comp(sec, "D1", "Device", "D_TVS", "SMF5.0A", "Diode_SMD:D_SOD-123F", {"1": "+5V", "2": "GND"}, lcsc="C193402")
R(sec, "R1", "5.1k", "CC1", "GND", "C27834")
R(sec, "R2", "5.1k", "CC2", "GND", "C27834")
comp(sec, "C1", "Device", "C_Polarized", "220uF 16V", "Capacitor_SMD:CP_Elec_6.3x7.7",
     {"1": "+5V", "2": "GND"}, lcsc="C2887273", note="bulk for DFPlayer/speaker peaks")
C(sec, "C2", "10uF 25V", "+5V", "GND", "C15850")
C(sec, "C13", "10uF 25V", "+5V", "GND", "C15850", note="near the DFPlayer sockets")
comp(sec, "#FLG01", "power", "PWR_FLAG", "PWR_FLAG", "", {"1": "+5V"})
comp(sec, "#FLG02", "power", "PWR_FLAG", "PWR_FLAG", "", {"1": "GND"})

# ------------------------------------------------------------------ 2. 3.3 V
sec = "2  3.3 V SUPPLY (MCU + RADIO + OLED)"
comp(sec, "U1", "Regulator_Linear", "AP2112K-3.3", "AP2112K-3.3", "Package_TO_SOT_SMD:SOT-23-5",
     {"1": "+5V", "2": "GND", "3": "+5V", "4": "NC", "5": "+3V3"}, lcsc="C51118")
C(sec, "C3", "10uF 25V", "+3V3", "GND", "C15850")
C(sec, "C4", "100nF", "+3V3", "GND", "C1711")
comp(sec, "D2", "Device", "LED", "green", "LED_SMD:LED_0805_2012Metric", {"1": "GND", "2": "PWR_LED"}, lcsc="C2297")
R(sec, "R3", "1k", "+3V3", "PWR_LED", "C17513")

# ------------------------------------------------------------------ 3. MCU
sec = "3  ATMEGA328P-AU (8 MHz crystal, 3.3 V)"
comp(sec, "U2", "MCU_Microchip_ATmega", "ATmega328P-A", "ATmega328P-AU",
     "Package_QFP:TQFP-32_7x7mm_P0.8mm",
     {"4": "+3V3", "6": "+3V3", "18": "+3V3", "3": "GND", "5": "GND", "21": "GND", "20": "AREF",
      "29": "RST", "7": "XTAL1", "8": "XTAL2",
      "13": "CE", "14": "CSN", "15": "MOSI", "16": "MISO", "17": "SCK", "32": "IRQ",
      "1": "BUZ", "2": "BTN_ACK", "9": "BTN_CLR", "10": "BTN_MUTE",
      "30": "DF_TXD", "31": "MCU_TX", "23": "DF_BUSY", "27": "SDA", "28": "SCL"}, lcsc="C14877")
C(sec, "C5", "100nF", "+3V3", "GND", "C1711")
C(sec, "C6", "100nF", "+3V3", "GND", "C1711")
C(sec, "C7", "100nF", "+3V3", "GND", "C1711", note="AVCC")
C(sec, "C8", "100nF", "AREF", "GND", "C1711")
comp(sec, "Y1", "Device", "Crystal_GND24", "8MHz", "Crystal:Crystal_SMD_3225-4Pin_3.2x2.5mm",
     {"1": "XTAL1", "2": "GND", "3": "XTAL2", "4": "GND"}, lcsc="C2905353")
C(sec, "C9", "18pF", "XTAL1", "GND", "C113825")
C(sec, "C10", "18pF", "XTAL2", "GND", "C113825")
R(sec, "R4", "10k", "+3V3", "RST", "C17414")
comp(sec, "SW1", "Switch", "SW_Push", "RESET", SW_FP, {"1": "RST", "2": "GND"}, lcsc="C318884")
comp(sec, "J3", "Connector_Generic", "Conn_02x03_Odd_Even", "ISP",
     "Connector_PinHeader_2.54mm:PinHeader_2x03_P2.54mm_Vertical",
     {"1": "MISO", "2": "+3V3", "3": "SCK", "4": "MOSI", "5": "RST", "6": "GND"}, lcsc="C56153599")

# ------------------------------------------------------------------ 4. radio
sec = "4  SI24R1 RADIO MODULE"
comp(sec, "U3", "Connector_Generic", "Conn_01x08", "SI24R1 module", "BellParts:SI24R1_Module_12x18",
     {"1": "+3V3", "2": "GND", "3": "CE", "4": "CSN", "5": "SCK", "6": "MOSI", "7": "MISO", "8": "IRQ"},
     external=True, note="user-supplied module (not on this BOM)")
C(sec, "C11", "100nF", "+3V3", "GND", "C1711", note="at the radio pins")
C(sec, "C12", "22uF", "+3V3", "GND", "C5674", note="TX/RX current pulses")

# ------------------------------------------------------------------ 5. OLED + buttons
sec = "5  OLED + BUTTONS"
comp(sec, "J4", "Connector_Generic", "Conn_01x04", "OLED SSD1306 I2C", SKT % 4,
     {"1": "GND", "2": "+3V3", "3": "SCL", "4": "SDA"}, lcsc="C2897367",
     note="common 0.96in module order GND VCC SCL SDA - CHECK your module")
R(sec, "R5", "4.7k", "+3V3", "SDA", "C17673", note="many OLED modules already have pull-ups; fit if yours has none")
R(sec, "R6", "4.7k", "+3V3", "SCL", "C17673", note="see R5")
comp(sec, "SW2", "Switch", "SW_Push", "ACK", SW_FP, {"1": "BTN_ACK", "2": "GND"}, lcsc="C318884")
comp(sec, "SW3", "Switch", "SW_Push", "CLEAR", SW_FP, {"1": "BTN_CLR", "2": "GND"}, lcsc="C318884")
comp(sec, "SW4", "Switch", "SW_Push", "MUTE", SW_FP, {"1": "BTN_MUTE", "2": "GND"}, lcsc="C318884")

# ------------------------------------------------------------------ 6. audio
sec = "6  AUDIO: DFPLAYER MINI + SPEAKER + BUZZER"
# DFPlayer Mini pins as commonly documented (verify with your module before ordering PCBs):
#   left column, top->bottom:  1 VCC  2 RX  3 TX  4 DAC_R  5 DAC_L  6 SPK2  7 GND  8 SPK1
#   right column, top->bottom: 16 BUSY 15 USB- 14 USB+ 13 ADKEY2 12 ADKEY1 11 IO2 10 GND 9 IO1
comp(sec, "J6", "Connector_Generic", "Conn_01x08", "DFPlayer left", SKT % 8,
     {"1": "+5V", "2": "DF_RXD", "3": "DF_TXD", "4": "NC", "5": "NC", "6": "SPK2", "7": "GND", "8": "SPK1"},
     lcsc="C2897371")
comp(sec, "J7", "Connector_Generic", "Conn_01x08", "DFPlayer right", SKT % 8,
     {"1": "NC", "2": "GND", "3": "NC", "4": "NC", "5": "NC", "6": "NC", "7": "NC", "8": "DF_BUSY"},
     lcsc="C2897371")
R(sec, "R7", "1k", "MCU_TX", "DF_RXD", "C17513", note="series resistor on the DFPlayer RX line")
comp(sec, "J8", "Connector_Generic", "Conn_01x02", "SPEAKER 4R 3W",
     "TerminalBlock:TerminalBlock_MaiXu_MX126-5.0-02P_1x02_P5.00mm", {"1": "SPK1", "2": "SPK2"}, lcsc="C474881")
comp(sec, "Q1", "Transistor_BJT", "MMBT3904", "MMBT3904", "Package_TO_SOT_SMD:SOT-23",
     {"1": "BUZ_B", "2": "GND", "3": "BUZ_C"}, lcsc="C81464")
R(sec, "R8", "1k", "BUZ", "BUZ_B", "C17513")
R(sec, "R9", "10k", "BUZ_B", "GND", "C17414")
comp(sec, "BZ1", "Device", "Buzzer", "active 5V", "Buzzer_Beeper:Buzzer_12x9.5RM7.6",
     {"1": "+5V", "2": "BUZ_C"}, lcsc="C96093", optional=True)
comp(sec, "D3", "Diode", "1N4148W", "1N4148W", "Diode_SMD:D_SOD-123", {"1": "+5V", "2": "BUZ_C"}, lcsc="C304038", optional=True)

# ------------------------------------------------------------------ 7. test points + mounting
sec = "7  TEST POINTS + MOUNTING"
for ref, net in [("TP1", "+5V"), ("TP2", "+3V3"), ("TP3", "GND"), ("TP4", "MCU_TX")]:
    comp(sec, ref, "Connector", "TestPoint", net, "TestPoint:TestPoint_Pad_D2.0mm", {"1": net})
for i in range(4):
    comp(sec, "H%d" % (i + 1), "Mechanical", "MountingHole", "M3", "MountingHole:MountingHole_3.2mm_M3", {})

# ================================================================== PCB data
BOARD = (100.0, 72.0)                      # mm, top-left origin
P = {
    # radio at the top-left, antenna hangs 6.5 mm over the top edge
    "U3": (20.0, 11.5, 0), "C11": (11.0, 16.0, 90), "C12": (29.0, 16.0, 90),
    # MCU in the middle
    "U2": (50.0, 30.0, 0),
    "C5": (41.0, 26.0, 90), "C6": (41.0, 34.0, 90), "C7": (59.0, 34.0, 90), "C8": (59.0, 27.0, 90),
    "Y1": (50.0, 42.5, 0), "C9": (45.5, 45.5, 0), "C10": (54.5, 45.5, 0),
    "R4": (57.0, 21.0, 0), "SW1": (68.0, 14.0, 0), "J3": (86.0, 10.0, 0),
    # OLED header and the three buttons along the top/right
    "J4": (42.0, 5.0, 90), "R5": (61.5, 8.5, 0), "R6": (61.5, 11.5, 0),
    "SW2": (68.0, 24.0, 0), "SW3": (78.0, 24.0, 0), "SW4": (88.0, 24.0, 0),
    # USB-C and power at the bottom-left
    "J1": (14.0, 68.6, 0), "R1": (25.0, 66.0, 0), "R2": (25.0, 69.0, 0),
    "F1": (24.0, 56.5, 0), "D1": (34.0, 56.5, 0), "C1": (13.0, 44.0, 0), "C2": (32.0, 63.0, 0),
    "U1": (44.0, 61.0, 0), "C3": (51.0, 64.5, 0), "C4": (51.0, 58.0, 0),
    "D2": (38.0, 69.0, 0), "R3": (42.0, 69.0, 0),
    "C13": (60.0, 62.0, 0),
    # DFPlayer sockets: two 1x8 columns 17.78 mm apart (module 20.8 x 20.8)
    "J6": (70.0, 38.0, 0), "J7": (87.78, 55.78, 180),
    "R7": (66.0, 66.0, 0),
    "J8": (84.0, 64.0, 0), "Q1": (73.0, 68.5, 0), "R8": (68.0, 69.5, 0), "R9": (78.0, 69.5, 0),
    "BZ1": (20.0, 36.0, 0), "D3": (33.0, 42.0, 0),
    "TP1": (4.0, 54.0, 0), "TP2": (4.0, 58.0, 0), "TP3": (4.0, 50.0, 0), "TP4": (4.0, 46.0, 0),
    "H1": (3.5, 3.5, 0), "H2": (96.5, 3.5, 0), "H3": (3.5, 68.5, 0), "H4": (96.5, 68.5, 0),
}
PADMAP = {"J1": {"SH": "S1"}}
KEEPOUTS = [(13.5, 0.0, 26.5, 10.2)]
TEXTS = [
    ("ANTENNA - KEEP CLEAR", 20.0, 5.0, 0.8, "F.SilkS", 0),
    ("OLED", 38.0, 8.0, 0.8, "F.SilkS", 0),
    ("RESET", 68.0, 18.5, 0.8, "F.SilkS", 0),
    ("ACK", 68.0, 28.5, 0.8, "F.SilkS", 0),
    ("CLEAR", 78.0, 28.5, 0.8, "F.SilkS", 0),
    ("MUTE", 88.0, 28.5, 0.8, "F.SilkS", 0),
    ("ISP", 86.0, 5.5, 0.8, "F.SilkS", 0),
    ("DFPLAYER MINI", 79.0, 35.5, 0.9, "F.SilkS", 0),
    ("SPEAKER 4R", 86.5, 60.5, 0.8, "F.SilkS", 0),
    ("USB-C 5V", 14.0, 53.0, 0.8, "F.SilkS", 0),
    ("Bell Receiver v1", 60.0, 52.0, 1.0, "F.SilkS", 0),
    ("github.com/ElectroIoT/Restaurant-Calling-Bell", 50.0, 36.0, 0.8, "B.SilkS", 0),
]
