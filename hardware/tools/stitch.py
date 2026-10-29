"""Add GND stitching vias on a grid wherever they clear all non-GND copper (rectangular boards).
Run with KiCad's python:  python.exe stitch.py remote|receiver   (adapted from NewYearPixelFireworks)
"""
import math, os, sys
import pcbnew
from pcbnew import FromMM, VECTOR2I, ToMM
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
D = __import__(sys.argv[1] + "_design")
PCB = os.path.join(HERE, "..", D.OUT_DIR, D.PROJECT + ".kicad_pcb")
OX = OY = 100.0
BW, BH = D.BOARD
VIA_D, DRILL, CLR = 0.6, 0.3, 0.45        # clearance incl. safety margin to any other-net copper
ANTS = [(OX + a, OY + b, OX + c, OY + d) for (a, b, c, d) in D.KEEPOUTS]


def inside(x, y, extra=0.0):
    return OX + 1.5 - extra <= x <= OX + BW - 1.5 + extra and OY + 1.5 - extra <= y <= OY + BH - 1.5 + extra


def in_ant(x, y):
    return any(a[0] - 1 <= x <= a[2] + 1 and a[1] - 1 <= y <= a[3] + 1 for a in ANTS)

def seg_dist(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    L = dx * dx + dy * dy
    t = 0 if L == 0 else max(0, min(1, ((px - ax) * dx + (py - ay) * dy) / L))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))

b = pcbnew.LoadBoard(PCB)
snapshot = [t for t in b.Tracks()]      # one snapshot: b.Tracks() breaks after Remove()
segs, pts, rects = [], [], []
for t in snapshot:
    if t.GetNetname() == "GND":
        if t.Type() == pcbnew.PCB_VIA_T:
            b.Remove(t)                 # start clean: drop earlier GND vias
        continue
    if t.Type() == pcbnew.PCB_VIA_T:
        p = t.GetPosition(); pts.append((ToMM(p.x), ToMM(p.y), ToMM(t.GetWidth(pcbnew.F_Cu)) / 2))
    else:
        a, e = t.GetStart(), t.GetEnd()
        segs.append((ToMM(a.x), ToMM(a.y), ToMM(e.x), ToMM(e.y), ToMM(t.GetWidth()) / 2))
rects_ng = []
for fp in b.GetFootprints():
    for pad in fp.Pads():
        if pad.GetNetname() == "GND":
            continue
        bb = pad.GetBoundingBox()
        rects_ng.append((ToMM(bb.GetLeft()), ToMM(bb.GetTop()), ToMM(bb.GetRight()), ToMM(bb.GetBottom())))
        rects.append((ToMM(bb.GetLeft()), ToMM(bb.GetTop()), ToMM(bb.GetRight()), ToMM(bb.GetBottom())))
    for pad in fp.Pads():       # unconnected/NC pads and holes must be avoided too
        if pad.GetNetname() == "GND":
            bb = pad.GetBoundingBox()
            rects.append((ToMM(bb.GetLeft()), ToMM(bb.GetTop()), ToMM(bb.GetRight()), ToMM(bb.GetBottom())))
    # keep stitching vias out of footprint courtyards that hold non-GND holes (mounting holes etc.)
gnd = b.FindNet("GND")
added = 0
gridvias = []
pitch = 5.0
for i in range(int(BW // pitch) + 1):
    for j in range(int(BH // pitch) + 1):
        x, y = OX + 2.0 + i * pitch, OY + 2.0 + j * pitch
        if not inside(x, y) or in_ant(x, y):
            continue
        r = VIA_D / 2 + CLR
        if any(seg_dist(x, y, *s[:4]) < s[4] + r for s in segs):
            continue
        if any(math.hypot(x - p[0], y - p[1]) < p[2] + r for p in pts):
            continue
        if any(x > q[0] - r and x < q[2] + r and y > q[1] - r and y < q[3] + r for q in rects):
            continue
        v = pcbnew.PCB_VIA(b)
        v.SetPosition(VECTOR2I(FromMM(x), FromMM(y)))
        v.SetWidth(FromMM(VIA_D)); v.SetDrill(FromMM(DRILL))
        v.SetNet(gnd); v.SetViaType(pcbnew.VIATYPE_THROUGH)
        b.Add(v); added += 1; gridvias.append((x, y))
def clear(x, y):
    r = VIA_D / 2 + CLR
    if not inside(x, y, 0.5) or in_ant(x, y):
        return False
    if any(seg_dist(x, y, *s[:4]) < s[4] + r for s in segs): return False
    if any(math.hypot(x - p[0], y - p[1]) < p[2] + r for p in pts): return False
    if any(x > q[0] - r and x < q[2] + r and y > q[1] - r and y < q[3] + r for q in nongnd_rects): return False
    if any(math.hypot(x - q[0], y - q[1]) < VIA_D + 0.4 for q in placed): return False
    return True

# second pass: one stitching via beside every GND pad, so no pour fragment holding a GND pad is isolated
nongnd_rects = [q for q in rects_ng]
placed = list(gridvias)
for fp in b.GetFootprints():
    for pad in fp.Pads():
        if pad.GetNetname() != "GND" or pad.GetAttribute() != pcbnew.PAD_ATTRIB_SMD:
            continue
        c = pad.GetPosition(); bb = pad.GetBoundingBox()
        hw = max(ToMM(bb.GetWidth()), ToMM(bb.GetHeight())) / 2
        done = False
        for d in (hw + 0.9, hw + 1.4, hw + 2.0):
            for ang in range(0, 360, 30):
                x = ToMM(c.x) + d * math.cos(math.radians(ang)); y = ToMM(c.y) + d * math.sin(math.radians(ang))
                if clear(x, y):
                    v = pcbnew.PCB_VIA(b)
                    v.SetPosition(VECTOR2I(FromMM(x), FromMM(y)))
                    v.SetWidth(FromMM(VIA_D)); v.SetDrill(FromMM(DRILL))
                    v.SetNet(gnd); v.SetViaType(pcbnew.VIATYPE_THROUGH)
                    b.Add(v); placed.append((x, y)); added += 1; done = True
                    break
            if done:
                break
pcbnew.ZONE_FILLER(b).Fill(b.Zones())

# third pass: any pour fragment (either layer) still without a GND via gets one inside it
def fragments_without_via():
    out = []
    gvias = [t for t in [t for t in b.Tracks()] if t.Type() == pcbnew.PCB_VIA_T and t.GetNetname() == "GND"]
    for z in b.Zones():
        if z.GetIsRuleArea() or z.GetNetname() != "GND":
            continue
        for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
            if not z.IsOnLayer(layer):
                continue
            ps = z.GetFilledPolysList(layer)
            for i in range(ps.OutlineCount()):
                if not any(ps.Contains(v.GetPosition(), i) for v in gvias):
                    out.append((ps, i))
    return out

for ps, i in fragments_without_via():
    bb = ps.COutline(i).BBox()
    x0, y0, w, h = ToMM(bb.GetX()), ToMM(bb.GetY()), ToMM(bb.GetWidth()), ToMM(bb.GetHeight())
    best = None
    steps = 12
    for a in range(steps + 1):
        for c in range(steps + 1):
            x, y = x0 + w * a / steps, y0 + h * c / steps
            ring = [(x + 0.45 * math.cos(k * math.pi / 4), y + 0.45 * math.sin(k * math.pi / 4)) for k in range(8)]
            if all(ps.Contains(VECTOR2I(FromMM(rx), FromMM(ry)), i) for rx, ry in ring) and clear(x, y):
                best = (x, y); break
        if best: break
    if best:
        v = pcbnew.PCB_VIA(b)
        v.SetPosition(VECTOR2I(FromMM(best[0]), FromMM(best[1])))
        v.SetWidth(FromMM(VIA_D)); v.SetDrill(FromMM(DRILL))
        v.SetNet(gnd); v.SetViaType(pcbnew.VIATYPE_THROUGH)
        b.Add(v); placed.append(best); added += 1
        print("island via at", [round(c, 2) for c in best])
    else:
        # last resort: via-in-pad on the GND pad(s) trapped in this fragment (0.5 mm via, 0.3 mm drill)
        n = 0
        for fp in b.GetFootprints():
            for pad in fp.Pads():
                if pad.GetNetname() == "GND" and pad.GetAttribute() == pcbnew.PAD_ATTRIB_SMD                         and ps.Contains(pad.GetPosition(), i):
                    v = pcbnew.PCB_VIA(b)
                    dx = 0.3 if fp.GetReference() == "U2" else 0.0     # keep clear of the USB D- track
                    v.SetPosition(VECTOR2I(pad.GetPosition().x + FromMM(dx), pad.GetPosition().y))
                    v.SetWidth(FromMM(0.5)); v.SetDrill(FromMM(0.3))
                    v.SetNet(gnd); v.SetViaType(pcbnew.VIATYPE_THROUGH)
                    b.Add(v); added += 1; n += 1
                    print("via-in-pad", fp.GetReference(), pad.GetNumber())
                    break
            if n:
                break
        if not n:
            print("UNRESOLVED fragment near", round(x0, 1), round(y0, 1), round(w, 1), round(h, 1))
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(PCB, b)
print("stitching vias added:", added)
