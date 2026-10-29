"""Generate BOM CSV files from the design files and the LCSC lookup table (parts.py).

  python gen_bom.py        -> hardware/remote/bom/, hardware/receiver/bom/, hardware/bom/
Files: remote-1x.csv, remote-10x.csv, receiver-1x.csv, system-complete.csv and BOM-SUMMARY.md.
Prices are LCSC website prices read on 2026-10-29 at the quantity tier noted per part (USD); they are
indicative and exclude shipping, tax and PCB fabrication. Parts without a verified price are labelled.
"""
import csv
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import receiver_design
import remote_design
from parts import PARTS

HW = os.path.join(HERE, "..")
COLS = ["Refs", "Qty", "Component", "Value", "Package", "MPN", "Manufacturer", "LCSC", "Stock 2026-10-29",
        "Unit USD", "Price tier", "Line USD", "Need", "Status", "Substitutes / notes"]

SUBS = {
    "C14877": "ATmega328P-AUR (C618816, reel); any ATmega328P TQFP-32 at 8 MHz-capable grade",
    "C1711": "any 100nF 0805 X7R >=16V (e.g. Yageo CC0805KRX7R9BB104)",
    "C15850": "any 10uF 0805 X5R >=16V; keep ceramic for the remote (leakage)",
    "C5674": "any 22uF 0805 X5R >=6.3V; for 3V use 10V rating to limit DC-bias loss",
    "C113825": "any 18pF C0G/NP0 0805",
    "C17513": "any 1% 0805 resistor", "C17709": "any 1% 0805 resistor", "C17414": "any 1% 0805 resistor",
    "C27834": "any 1% 0805 resistor", "C17673": "any 1% 0805 resistor", "C17477": "solder bridge for 0R",
    "C51118": "Tech Public AP2112K-3.3TRG1 clone C23380830; or any 3.3V LDO >=300mA in SOT-23-5 with the same pinout",
    "C193402": "Littelfuse SMF5.0A C151296",
    "C81464": "any SOT-23 NPN (MMBT3904, S8050)",
    "C2295": "any 0805 LED + re-check resistor", "C2297": "any 0805 LED + re-check resistor",
    "C318884": "any 5.1x5.1mm 4-pad SMD tact switch (footprint SW_TS-1187A)",
    "C96093": "any 12mm 3-5V active (self-oscillating) buzzer; or omit",
    "C2897371": "any 1x8 2.54mm female header", "C2897367": "any 1x4 2.54mm female header",
}

EXTERNAL_REMOTE = [
    ("U2", 1, "SI24R1 2.4GHz module (HC-24L01-M style, 12x18mm, 8 half-holes)", "", "12x18mm module", "n/a", "n/a",
     "owned by user", "", "", "", "", "Essential", "external", "any module with the same pinout: 3V3 GND CE CSN SCK MOSI MISO IRQ"),
    ("BT1", 1, "CR2450 coin cell (or 2xAAA alkaline + holder, never both)", "3V", "CR2450", "n/a", "n/a", "not looked up", "",
     "", "", "", "Essential", "unverified", "CR2032 is not recommended (lower capacity, higher internal resistance under pulses)"),
    ("-", 1, "Enclosure with button window / 3D-printed case", "", "", "n/a", "n/a", "not looked up", "", "", "", "", "Essential", "unverified", ""),
]
EXTERNAL_RECEIVER = [
    ("U3", 1, "SI24R1 2.4GHz module (12x18mm, 8 half-holes)", "", "12x18mm module", "n/a", "n/a", "owned by user", "", "", "", "", "Essential", "external", ""),
    ("U4", 1, "DFPlayer Mini MP3 module MP3-TF-16P (Robocraze SKU TIFEC0029, Rs 94 sale price incl. GST on 2026-10-29)", "3.2-5V", "21x21x10mm, 2x8 pins", "MP3-TF-16P", "generic", "n/a (Robocraze)", "", "", "", "", "Essential", "verify", "listing gives size and supply only; a buyer review says the board has NO pin labels: confirm pin order and row spacing on the real module before ordering PCBs; buy 2 spares"),
    ("DS1", 1, "0.96in SSD1306 OLED, I2C, 4-pin", "128x64", "module", "n/a", "n/a", "not looked up", "", "", "", "", "Essential", "unverified", "check pin order (GND VCC SCL SDA vs VCC GND SDA SCL)"),
    ("LS1", 1, "Speaker 4 ohm 3 W", "4R 3W", "", "n/a", "n/a", "not looked up", "", "", "", "", "Essential", "unverified", "DFPlayer Mini internal amp is rated for 3 W into 3 ohm at 5 V; 4 ohm gives less. See docs/power-and-safety.md"),
    ("-", 1, "microSD card (FAT32, up to 32 GB) with /MP3/0001..0014.mp3", "", "", "n/a", "n/a", "not looked up", "", "", "", "", "Essential", "unverified", ""),
    ("-", 1, "5V 2A USB charger + USB-C cable", "5V 2A", "", "n/a", "n/a", "not looked up", "", "", "", "", "Essential", "unverified", ""),
]


def pkg(fp):
    return fp.split(":")[-1].replace("_Metric", "") if fp else ""


def collect(design):
    """-> list of grouped rows (dicts)."""
    groups = {}
    for sec in design.S.values():
        for c in sec:
            if c["lib"] in ("power", "Mechanical", "Connector") and c["sym"] in ("PWR_FLAG", "MountingHole", "TestPoint"):
                continue
            if c.get("external") or not c.get("lcsc"):
                continue
            key = (c["lcsc"], c["value"], c["fp"], c.get("optional", False))
            g = groups.setdefault(key, dict(refs=[], c=c))
            g["refs"].append(c["ref"])
    rows = []
    for (lcsc, value, fp, opt), g in groups.items():
        p = PARTS[lcsc]
        qty = len(g["refs"])
        c = g["c"]
        note = "; ".join(x for x in [p.get("note"), c.get("note"), c.get("dnp_note")] if x)
        sub = SUBS.get(lcsc, "")
        rows.append(dict(refs=" ".join(sorted(g["refs"], key=lambda r: (len(r), r))), qty=qty, comp=p["desc"],
                         value=value, pkg=pkg(fp), mpn=p["mpn"], mfr=p["mfr"], lcsc=lcsc, stock=p["stock"],
                         unit=p["price_usd"], tier=p["tier"], need="Optional" if opt else "Essential",
                         status=p["status"], notes=(sub + ("; " if sub and note else "") + note)))
    rows.sort(key=lambda r: (r["need"], r["refs"]))
    return rows


def to_lines(rows, mult=1):
    out = []
    for r in rows:
        q = r["qty"] * mult
        out.append([r["refs"], q, r["comp"], r["value"], r["pkg"], r["mpn"], r["mfr"], r["lcsc"], r["stock"],
                    "%.4f" % r["unit"], r["tier"], "%.2f" % (q * r["unit"]), r["need"], r["status"], r["notes"]])
    return out


def total(rows, mult=1, only_ok=False):
    return sum(r["qty"] * mult * r["unit"] for r in rows if (not only_ok or r["status"] == "ok"))


def write_csv(path, lines):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(COLS)
        w.writerows(lines)


def ext_lines(ext, mult=1):
    return [[r[0], r[1] * mult] + list(r[2:]) for r in ext]


rem, rec = collect(remote_design), collect(receiver_design)

write_csv(os.path.join(HW, "remote", "bom", "remote-1x.csv"), to_lines(rem) + ext_lines(EXTERNAL_REMOTE))
write_csv(os.path.join(HW, "remote", "bom", "remote-10x.csv"), to_lines(rem, 10) + ext_lines(EXTERNAL_REMOTE, 10))
write_csv(os.path.join(HW, "receiver", "bom", "receiver-1x.csv"), to_lines(rec) + ext_lines(EXTERNAL_RECEIVER))

# complete system = 1 receiver + 10 remotes (fitted-quantity of polarity jumpers is 2 of 4 per remote)
sysl = [["[RECEIVER] " + l[0]] + l[1:] for l in to_lines(rec) + ext_lines(EXTERNAL_RECEIVER)]
sysl += [["[REMOTE x10] " + l[0]] + l[1:] for l in to_lines(rem, 10) + ext_lines(EXTERNAL_REMOTE, 10)]
write_csv(os.path.join(HW, "bom", "system-complete.csv"), sysl)

t_rem, t_rec = total(rem), total(rec)
t_rem_ess = sum(r["qty"] * r["unit"] for r in rem if r["need"] == "Essential")
t_rec_ess = sum(r["qty"] * r["unit"] for r in rec if r["need"] == "Essential")
unverified = sorted({r["lcsc"] for r in rem + rec if r["status"] != "ok"})

md = []
md.append("# BOM summary (LCSC prices read 2026-10-29)\n")
md.append("Only parts that were looked up on lcsc.com appear with a price. Modules (SI24R1, DFPlayer Mini, OLED), the speaker,")
md.append("coin cell, microSD card and enclosure are NOT priced: they were either owned already or not looked up.\n")
md.append("| Board | LCSC parts, essential (USD) | LCSC parts incl. optional (USD) | Lines |")
md.append("|---|---|---|---|")
md.append("| Remote (1 pc) | %.2f | %.2f | %d |" % (t_rem_ess, t_rem, len(rem)))
md.append("| Remote (10 pcs) | %.2f | %.2f | %d |" % (t_rem_ess * 10, t_rem * 10, len(rem)))
md.append("| Receiver (1 pc) | %.2f | %.2f | %d |" % (t_rec_ess, t_rec, len(rec)))
md.append("| **System: 1 receiver + 10 remotes** | **%.2f** | **%.2f** | |" % (t_rec_ess + 10 * t_rem_ess, t_rec + 10 * t_rem))
md.append("\nUnit prices use the quantity tier shown in the CSV (often a 5+/20+/100+ tier, so 1-piece prices are higher).")
md.append("Not included: PCB fabrication, shipping, import duty/tax, modules, battery, enclosure.\n")
md.append("## Parts that need checking before ordering\n")
md.append("| LCSC | MPN | Why |")
md.append("|---|---|---|")
for l in unverified:
    p = PARTS[l]
    md.append("| %s | %s | %s |" % (l, p["mpn"], p.get("note", "")))
md.append("")
os.makedirs(os.path.join(HW, "bom"), exist_ok=True)
open(os.path.join(HW, "bom", "BOM-SUMMARY.md"), "w", encoding="utf-8", newline="\n").write("\n".join(md))
print("remote 1x essential %.2f / total %.2f; receiver essential %.2f / total %.2f" % (t_rem_ess, t_rem, t_rec_ess, t_rec))
print("system essential %.2f, with optional %.2f" % (t_rec_ess + 10 * t_rem_ess, t_rec + 10 * t_rem))
