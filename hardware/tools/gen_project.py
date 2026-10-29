"""Write <board>/kicad/<project>.kicad_pro and fp-lib-table (project footprint library + net classes).

Usage: python gen_project.py remote|receiver
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
D = __import__(sys.argv[1] + "_design")
OUT = os.path.join(HERE, "..", D.OUT_DIR)
os.makedirs(OUT, exist_ok=True)

LIB_TABLE = '''(fp_lib_table
  (version 7)
  (lib (name "BellParts")(type "KiCad")(uri "${KIPRJMOD}/../../lib/BellParts.pretty")(options "")(descr "Project footprints"))
)
'''
open(os.path.join(OUT, "fp-lib-table"), "w", newline="\n").write(LIB_TABLE)


def cls(name, track, clr=0.2, via=0.6, drill=0.3, prio=None):
    c = dict(bus_width=12, clearance=clr, diff_pair_gap=0.25, diff_pair_via_gap=0.25, diff_pair_width=0.2,
             line_style=0, microvia_diameter=0.3, microvia_drill=0.1, name=name, pcb_color="rgba(0, 0, 0, 0.000)",
             schematic_color="rgba(0, 0, 0, 0.000)", track_width=track, via_diameter=via, via_drill=drill,
             wire_width=6)
    if prio is not None:
        c["priority"] = prio
    return c


if sys.argv[1] == "remote":
    classes = [cls("Default", 0.25), cls("Power", 0.4, prio=1)]
    patterns = [{"netclass": "Power", "pattern": "+3V0"}]
else:
    classes = [cls("Default", 0.25), cls("Power5V", 0.8, prio=1), cls("Power3V3", 0.5, prio=2)]
    patterns = [{"netclass": "Power5V", "pattern": "+5V"}, {"netclass": "Power5V", "pattern": "VBUS"},
                {"netclass": "Power3V3", "pattern": "+3V3"}]

pro = {
    "meta": {"filename": D.PROJECT + ".kicad_pro", "version": 3},
    "board": {"design_settings": {"rules": {"min_clearance": 0.2, "min_track_width": 0.2,
                                            "min_via_diameter": 0.5, "min_through_hole_diameter": 0.3,
                                            "min_copper_edge_clearance": 0.3}}},
    "net_settings": {"classes": classes, "meta": {"version": 3}, "netclass_assignments": None,
                     "netclass_patterns": patterns},
    "libraries": {"pinned_footprint_libs": [], "pinned_symbol_libs": []},
    "schematic": {"drawing": {"default_line_thickness": 6.0}},
}
json.dump(pro, open(os.path.join(OUT, D.PROJECT + ".kicad_pro"), "w"), indent=2)
print("project files written for", D.PROJECT)
