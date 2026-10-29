"""Generate <board>/kicad/<project>.kicad_sch from <board>_design.py (every pin gets a stub + net label/power symbol).

Usage: python gen_schematic.py remote|receiver   (adapted from the NewYearPixelFireworks generator)
"""
import math, os, re, sys, uuid
sys.path.insert(0, os.path.dirname(__file__))
import kilib
import importlib
_D = importlib.import_module(sys.argv[1] + "_design")
S, PROJECT = _D.S, _D.PROJECT
POWER_NETS = {"GND": "GND", "+5V": "+5V", "+3V3": "+3V3", "+3V0": "+3V0"}
TITLE = _D.TITLE
COMMENT = getattr(_D, "COMMENT", "")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", _D.OUT_DIR, PROJECT + ".kicad_sch")
STUB = 2.54
GRID = 2.54


def uid():
    return str(uuid.uuid4())


ROOT = uid()
FONT = "(effects (font (size 1.27 1.27)))"


def snap(v):
    return round(v / GRID) * GRID


def f(v):
    return ("%.2f" % v).rstrip("0").rstrip(".") if "." in ("%.2f" % v) else "%.2f" % v


used_syms = {}          # (lib, sym) -> text
def need_symbol(lib, sym):
    key = (lib, sym)
    if key not in used_syms:
        txt = kilib.get_symbol(lib, sym)
        txt = txt.replace('(symbol "%s"' % sym, '(symbol "%s:%s"' % (lib, sym), 1)
        used_syms[key] = txt
    return key


def stacked(pins):
    """Group pins by connection point (x,y); returns {(x,y): [pin,...]}."""
    g = {}
    for p in pins:
        g.setdefault((round(p["x"], 2), round(p["y"], 2)), []).append(p)
    return g


def outward(angle):
    """Sheet-space unit vector pointing away from the symbol body for a pin of the given lib angle."""
    a = math.radians(angle + 180)
    return round(math.cos(a)), -round(math.sin(a))


def label_angle(dx, dy):
    return {(1, 0): 0, (0, -1): 90, (-1, 0): 180, (0, 1): 270}[(dx, dy)]


GND_ROT = {(0, 1): 0, (1, 0): 90, (0, -1): 180, (-1, 0): 270}
UP_ROT = {(0, -1): 0, (-1, 0): 90, (0, 1): 180, (1, 0): 270}   # +5V, +3V3, PWR_FLAG

body = []        # wires, labels, symbols, no_connects, texts
pwr_count = [0]
flag_count = [0]


def wire(x1, y1, x2, y2):
    body.append('\t(wire (pts (xy %s %s) (xy %s %s)) (stroke (width 0) (type solid)) (uuid "%s"))'
                % (f(x1), f(y1), f(x2), f(y2), uid()))


def power_symbol(net, x, y, d):
    pwr_count[0] += 1
    rot = GND_ROT[d] if net == "GND" else UP_ROT[d]
    key = need_symbol("power", net)
    body.append(
        '\t(symbol (lib_id "power:%s") (at %s %s %d) (unit 1) (exclude_from_sim no) (in_bom no) (on_board no) (dnp no)\n'
        '\t\t(uuid "%s")\n'
        '\t\t(property "Reference" "#PWR%03d" (at %s %s 0) (hide yes) %s)\n'
        '\t\t(property "Value" "%s" (at %s %s 0) %s)\n'
        '\t\t(property "Footprint" "" (at %s %s 0) (hide yes) %s)\n'
        '\t\t(property "Datasheet" "" (at %s %s 0) (hide yes) %s)\n'
        '\t\t(pin "1" (uuid "%s"))\n'
        '\t\t(instances (project "%s" (path "/%s" (reference "#PWR%03d") (unit 1))))\n\t)'
        % (net, f(x), f(y), rot, uid(), pwr_count[0], f(x), f(y), FONT, net, f(x + 2.54), f(y), FONT,
           f(x), f(y), FONT, f(x), f(y), FONT, uid(), PROJECT, ROOT, pwr_count[0]))


def net_label(net, x, y, d):
    ang = label_angle(*d)
    just = "left bottom" if ang in (0, 90) else "right bottom"
    body.append('\t(label "%s" (at %s %s %d) (effects (font (size 1.27 1.27)) (justify %s)) (uuid "%s"))'
                % (net, f(x), f(y), ang, just, uid()))


def no_connect(x, y):
    body.append('\t(no_connect (at %s %s) (uuid "%s"))' % (f(x), f(y), uid()))


def place(c, ix, iy):
    lib, sym = c["lib"], c["sym"]
    need_symbol(lib, sym)
    is_power_flag = lib == "power"
    pins = kilib.symbol_pins(lib, sym)
    groups = stacked(pins)
    # connection points
    for (px, py), plist in groups.items():
        x, y = ix + px, iy - py
        nets = {c["nets"].get(p["num"], "NC") for p in plist}
        if len(nets) > 1:
            raise SystemExit("stacked pins %s of %s have different nets %s" % ([p["num"] for p in plist], c["ref"], nets))
        net = nets.pop()
        d = outward(plist[0]["a"])
        if net == "NC":
            no_connect(x, y)
            continue
        ex, ey = x + d[0] * STUB, y + d[1] * STUB
        wire(x, y, ex, ey)
        if net in POWER_NETS:
            power_symbol(net, ex, ey, d)
        else:
            net_label(net, ex, ey, d)
    ys = [p["y"] for p in pins] or [0]
    top, bot = iy - max(ys) - 3.5, iy - min(ys) + 3.5
    ref = c["ref"]
    hide = "(hide yes)" if lib == "power" else ""
    body.append(
        '\t(symbol (lib_id "%s:%s") (at %s %s 0) (unit 1) (exclude_from_sim no) (in_bom %s) (on_board %s) (dnp no)\n'
        '\t\t(uuid "%s")\n'
        '\t\t(property "Reference" "%s" (at %s %s 0) %s)\n'
        '\t\t(property "Value" "%s" (at %s %s 0) %s)\n'
        '\t\t(property "Footprint" "%s" (at %s %s 0) (hide yes) %s)\n'
        '\t\t(property "Datasheet" "" (at %s %s 0) (hide yes) %s)\n'
        '%s'
        '\t\t(instances (project "%s" (path "/%s" (reference "%s") (unit 1))))\n\t)'
        % (lib, sym, f(ix), f(iy), "no" if is_power_flag else "yes", "no" if is_power_flag else "yes", uid(),
           ref, f(ix), f(top), FONT, c["value"], f(ix), f(bot), FONT, c["fp"], f(ix), f(iy), FONT,
           f(ix), f(iy), FONT,
           "".join('\t\t(pin "%s" (uuid "%s"))\n' % (p["num"], uid()) for p in pins),
           PROJECT, ROOT, ref))


def bbox(c):
    pins = kilib.symbol_pins(c["lib"], c["sym"])
    if not pins:
        return 10, 10
    xs = [p["x"] for p in pins]
    ys = [p["y"] for p in pins]
    return max(xs) - min(xs), max(ys) - min(ys)


# -------------------------------------------------------------------- layout
MARGIN_X, WIDTH, LBL = 20.0, 800.0, 26.0
y = 30.0
for section, comps in S.items():
    body.append('\t(text "%s" (exclude_from_sim no) (at %s %s 0) (effects (font (size 3.5 3.5) (bold yes)) (justify left bottom)) (uuid "%s"))'
                % (section, f(MARGIN_X), f(y), uid()))
    y += 12
    x, rowh = MARGIN_X, 0.0
    for c in comps:
        w, h = bbox(c)
        cw = w + 2 * LBL
        ch = h + 22
        if x + cw > MARGIN_X + WIDTH and x > MARGIN_X:
            x = MARGIN_X
            y += rowh
            rowh = 0.0
        pins = kilib.symbol_pins(c["lib"], c["sym"])
        minx = min([p["x"] for p in pins] or [0])
        maxy = max([p["y"] for p in pins] or [0])
        ix = snap(x + LBL - minx)
        iy = snap(y + 8 + maxy)
        place(c, ix, iy)
        x += cw
        rowh = max(rowh, ch)
    y += rowh + 14

paper = "A1" if y < 570 else "A0"
if paper == "A0" and y > 820:
    print("WARNING: layout taller than A0:", y)

libs = "\n".join(used_syms[k] for k in sorted(used_syms))
out = (
    '(kicad_sch\n\t(version 20250610)\n\t(generator "eeschema")\n\t(generator_version "10.0")\n'
    '\t(uuid "%s")\n\t(paper "%s")\n'
    '\t(title_block (title "%s") (date "2026-10-29") (rev "A")\n'
    '\t\t(company "ElectroIoT") (comment 1 "%s") (comment 2 "Generated by hardware/tools/gen_schematic.py from %s_design.py"))\n'
    '\t(lib_symbols\n%s\n\t)\n%s\n\t(sheet_instances (path "/" (page "1")))\n\t(embedded_fonts no)\n)\n'
    % (ROOT, paper, TITLE, COMMENT, sys.argv[1], libs, "\n".join(body))
)
os.makedirs(os.path.dirname(OUT), exist_ok=True)
open(OUT, "w", encoding="utf-8", newline="\n").write(out)
print("wrote", os.path.abspath(OUT), "paper", paper, "height", round(y), "symbols", sum(len(v) for v in S.values()))
