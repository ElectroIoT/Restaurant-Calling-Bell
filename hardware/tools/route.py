"""Auto-route with Freerouting 1.9.0 (headless) and import the result. Run with KiCad's python.

Usage: python route.py remote|receiver [passes]
Needs hardware/tools/_freerouting/v1.9.0/freerouting-1.9.0.jar (official release, not committed).
Auto-routing is a starting point: review every net before fabrication (docs/pcb-review.md).
"""
import os
import subprocess
import sys

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
D = __import__(sys.argv[1] + "_design")
KDIR = os.path.join(HERE, "..", D.OUT_DIR)
PCB = os.path.join(KDIR, D.PROJECT + ".kicad_pcb")
DSN, SES = os.path.join(KDIR, D.PROJECT + ".dsn"), os.path.join(KDIR, D.PROJECT + ".ses")
FR = os.path.join(HERE, "_freerouting", "v1.9.0", "freerouting-1.9.0.jar")
passes = sys.argv[2] if len(sys.argv) > 2 else "40"

b = pcbnew.LoadBoard(PCB)
assert pcbnew.ExportSpecctraDSN(b, DSN), "DSN export failed"
if os.path.exists(SES):
    os.remove(SES)
r = subprocess.run(["java", "-jar", FR, "-de", DSN, "-do", SES, "-mp", passes], capture_output=True, text=True)
print(r.stdout[-1200:], r.stderr[-400:])
assert os.path.exists(SES), "no session file written"
b = pcbnew.LoadBoard(PCB)
assert pcbnew.ImportSpecctraSES(b, SES), "SES import failed"
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(PCB, b)
print("routed board saved; tracks:", len(b.GetTracks()))
