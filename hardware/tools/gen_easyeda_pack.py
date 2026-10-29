"""Generate EasyEDA-friendly files from the same design source used for KiCad.

  python gen_easyeda_pack.py   ->  hardware/<board>/easyeda/{netlist.csv, nets-by-pin.csv, bom-easyeda.csv, placement-hint.csv}

The user lays the boards out in EasyEDA Pro (web). These files are the schematic data (what connects to what),
the BOM with LCSC part numbers in EasyEDA's "Supplier Part" convention, and a placement starting point taken from
the KiCad reference layout. They contain no EasyEDA-specific library data.
"""
import csv
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import receiver_design
import remote_design
from parts import PARTS

SKIP = ("PWR_FLAG", "MountingHole")


def fp_name(fp):
    return fp.split(":")[-1] if fp else ""


def pack(design, board):
    out = os.path.join(HERE, "..", board, "easyeda")
    os.makedirs(out, exist_ok=True)
    comps = [c for sec in design.S.values() for c in sec if c["sym"] not in SKIP]

    # netlist: net -> list of REF.PIN
    nets = {}
    for c in comps:
        for pin, net in c["nets"].items():
            if net != "NC":
                nets.setdefault(net, []).append("%s.%s" % (c["ref"], pin))
    with open(os.path.join(out, "netlist.csv"), "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Net", "Pin count", "Connections (REF.PIN)"])
        for net in sorted(nets, key=lambda n: (n not in ("GND", "+3V0", "+3V3", "+5V"), n)):
            pins = sorted(nets[net], key=lambda s: (s.split(".")[0].rstrip("0123456789"), int(''.join(ch for ch in s.split(".")[0] if ch.isdigit()) or 0), s))
            w.writerow([net, len(pins), " ".join(pins)])

    # per component pin table
    with open(os.path.join(out, "nets-by-pin.csv"), "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Ref", "Part / value", "Pin", "Net"])
        for c in comps:
            for pin, net in sorted(c["nets"].items(), key=lambda kv: (len(kv[0]), kv[0])):
                w.writerow([c["ref"], c["value"], pin, net])

    # BOM in EasyEDA import form (Designator, Footprint, Quantity, Value, Supplier Part)
    groups = {}
    for c in comps:
        if c.get("external") or c["sym"] in ("TestPoint",) or not c.get("lcsc"):
            continue
        key = (c["lcsc"], c["value"], fp_name(c["fp"]))
        groups.setdefault(key, []).append(c["ref"])
    with open(os.path.join(out, "bom-easyeda.csv"), "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Designator", "Footprint", "Quantity", "Value", "Manufacturer Part", "Manufacturer", "Supplier Part", "Supplier"])
        for (lcsc, value, fp), refs in sorted(groups.items(), key=lambda kv: kv[1][0]):
            p = PARTS[lcsc]
            w.writerow([",".join(sorted(refs, key=lambda r: (len(r), r))), fp, len(refs), value, p["mpn"], p["mfr"], lcsc, "LCSC"])

    # placement starting point (KiCad reference layout, board top-left origin, +y down, mm)
    with open(os.path.join(out, "placement-hint.csv"), "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Ref", "X mm", "Y mm", "Rotation deg", "Side", "Note"])
        for ref, pl in sorted(design.P.items()):
            side = "bottom" if len(pl) > 3 and pl[3] == "B" else "top"
            w.writerow([ref, pl[0], pl[1], pl[2], side, ""])
    print("wrote", os.path.normpath(out), "- nets:", len(nets), "parts:", len(comps))


pack(remote_design, "remote")
pack(receiver_design, "receiver")
