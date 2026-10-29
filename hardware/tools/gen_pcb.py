"""Build <board>/kicad/<project>.kicad_pcb from <board>_design.py with KiCad's own Python (pcbnew).

Run:  "%LOCALAPPDATA%\\Programs\\KiCad\\10.0\\bin\\python.exe" gen_pcb.py remote|receiver
Creates: outline, placed footprints with nets, GND pours, antenna keep-out, silkscreen.
Routing is done by route.py (Freerouting). Adapted from the NewYearPixelFireworks generator.
"""
import os
import sys

import pcbnew
from pcbnew import FromMM, VECTOR2I

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
D = __import__(sys.argv[1] + "_design")
S, P, PROJECT = D.S, D.P, D.PROJECT
OUT = os.path.join(HERE, "..", D.OUT_DIR, PROJECT + ".kicad_pcb")
FP_ROOT = os.path.expandvars(r"%LOCALAPPDATA%\Programs\KiCad\10.0\share\kicad\footprints")
LOCAL_LIB = os.path.join(HERE, "..", "lib", "BellParts.pretty")
W, H = D.BOARD
OX, OY = 100.0, 100.0                     # board top-left on the sheet


def mm(x, y):
    return VECTOR2I(FromMM(OX + x), FromMM(OY + y))


board = pcbnew.BOARD()
board.SetCopperLayerCount(2)

netinfo = {}


def get_net(name):
    if name not in netinfo:
        n = pcbnew.NETINFO_ITEM(board, name)
        board.Add(n)
        netinfo[name] = n
    return netinfo[name]


for sec in S.values():
    for c in sec:
        if c["lib"] == "power":
            continue
        for net in c["nets"].values():
            if net != "NC":
                get_net(net)

placed = {}
for sec in S.values():
    for c in sec:
        if c["lib"] == "power":
            continue
        lib, name = c["fp"].split(":")
        libdir = LOCAL_LIB if lib == "BellParts" else os.path.join(FP_ROOT, lib + ".pretty")
        fp = pcbnew.FootprintLoad(libdir, name)
        if fp is None:
            raise SystemExit("footprint not found: " + c["fp"])
        fp.SetReference(c["ref"])
        fp.SetValue(c["value"])
        pl = P[c["ref"]]
        x, y, rot = pl[0], pl[1], pl[2]
        fp.SetPosition(mm(x, y))
        fp.SetOrientationDegrees(rot)
        board.Add(fp)                       # flip needs the footprint to belong to a board
        if len(pl) > 3 and pl[3] == "B":
            try:
                fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
            except Exception:
                fp.Flip(fp.GetPosition(), True)
        pm = D.PADMAP.get(c["ref"], {})
        inv = {pm.get(k, k): v for k, v in c["nets"].items()}
        for pad in fp.Pads():
            n = inv.get(pad.GetNumber())
            if c["ref"] == "J1" and pad.GetNumber() == "" and pad.GetAttribute() == pcbnew.PAD_ATTRIB_PTH:
                n = "GND"                              # USB-C shield tabs
            if n and n != "NC":
                pad.SetNet(get_net(n))
        fp.Reference().SetTextSize(VECTOR2I(FromMM(0.8), FromMM(0.8)))
        fp.Reference().SetTextThickness(FromMM(0.12))
        fp.Value().SetVisible(False)
        for pad in fp.Pads():
            if pad.GetDrillSize().x and pad.GetDrillSize().x < FromMM(0.3):
                pad.SetDrillSize(VECTOR2I(FromMM(0.3), FromMM(0.3)))
        placed[c["ref"]] = fp

# ---------------------------------------------------------------- outline: rectangle
pts = [(0, 0), (W, 0), (W, H), (0, H)]
for i in range(4):
    a, b = pts[i], pts[(i + 1) % 4]
    seg = pcbnew.PCB_SHAPE(board)
    seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
    seg.SetLayer(pcbnew.Edge_Cuts)
    seg.SetWidth(FromMM(0.1))
    seg.SetStart(mm(*a))
    seg.SetEnd(mm(*b))
    board.Add(seg)

# ---------------------------------------------------------------- GND pours, both sides
for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
    z = pcbnew.ZONE(board)
    z.SetLayer(layer)
    z.SetNet(get_net("GND"))
    z.SetMinThickness(FromMM(0.25))
    z.SetLocalClearance(FromMM(0.3))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL)
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    ol = z.Outline()
    ol.NewOutline()
    for (px, py) in [(0.6, 0.6), (W - 0.6, 0.6), (W - 0.6, H - 0.6), (0.6, H - 0.6)]:
        ol.Append(FromMM(OX + px), FromMM(OY + py))
    board.Add(z)

# ---------------------------------------------------------------- antenna / module keep-out (all copper layers)
for k, (x0, y0, x1, y1) in enumerate(D.KEEPOUTS):
    ant = pcbnew.ZONE(board)
    ant.SetIsRuleArea(True)
    ant.SetDoNotAllowZoneFills(True)
    ant.SetDoNotAllowTracks(True)
    ant.SetDoNotAllowVias(True)
    ant.SetDoNotAllowPads(False)
    ant.SetDoNotAllowFootprints(False)
    ant.SetLayerSet(pcbnew.LSET.AllCuMask())
    ant.SetZoneName("antenna keepout %d" % (k + 1))
    ao = ant.Outline()
    ao.NewOutline()
    for (px, py) in [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]:
        ao.Append(FromMM(OX + px), FromMM(OY + py))
    board.Add(ant)

# ---------------------------------------------------------------- silkscreen
for (s, x, y, size, layer, rot) in D.TEXTS:
    t = pcbnew.PCB_TEXT(board)
    t.SetText(s)
    t.SetPosition(mm(x, y))
    lid = {"F.SilkS": pcbnew.F_SilkS, "B.SilkS": pcbnew.B_SilkS}[layer]
    t.SetLayer(lid)
    t.SetTextSize(VECTOR2I(FromMM(size), FromMM(size)))
    t.SetTextThickness(FromMM(max(0.12, size * 0.15)))
    t.SetTextAngleDegrees(rot)
    if layer.startswith("B."):
        t.SetMirrored(True)
    board.Add(t)

ds = board.GetDesignSettings()
ds.m_TrackMinWidth = FromMM(0.2)
ds.m_MinClearance = FromMM(0.2)
ds.m_ViasMinSize = FromMM(0.5)
ds.m_ViasMinDrill = FromMM(0.3)
ds.SetCustomTrackWidth(FromMM(0.25))

os.makedirs(os.path.dirname(OUT), exist_ok=True)
pcbnew.SaveBoard(OUT, board)
print("wrote", os.path.abspath(OUT), "footprints:", len(placed), "nets:", len(netinfo))
