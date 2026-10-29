"""Generate the project footprint library hardware/lib/BellParts.pretty.

Dimensions come from the vendor drawings saved in docs/datasheets (module) and the LCSC datasheets
for the holder and tact switch (see docs/hardware-verification.md for what is verified vs assumed).
Origins are chosen so placement coordinates in the design files are easy to reason about.
"""
import os

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "lib", "BellParts.pretty")
os.makedirs(OUT, exist_ok=True)

FONT = "(effects (font (size 1 1) (thickness 0.15)))"


def fnum(v):
    return ("%.3f" % v).rstrip("0").rstrip(".")


def header(name, descr, attr="smd"):
    return (
        '(footprint "%s"\n\t(version 20240108)\n\t(generator "bell_gen")\n\t(layer "F.Cu")\n'
        '\t(descr "%s")\n'
        '\t(property "Reference" "REF**" (at 0 -2 0) (layer "F.SilkS") %s)\n'
        '\t(property "Value" "%s" (at 0 2 0) (layer "F.Fab") %s)\n'
        '\t(attr %s)\n' % (name, descr, FONT, name, FONT, attr))


def rect(x0, y0, x1, y1, layer, width=0.12):
    return ('\t(fp_rect (start %s %s) (end %s %s) (stroke (width %s) (type solid)) (fill none) (layer "%s"))\n'
            % (fnum(x0), fnum(y0), fnum(x1), fnum(y1), fnum(width), layer))


def line(x0, y0, x1, y1, layer, width=0.12):
    return ('\t(fp_line (start %s %s) (end %s %s) (stroke (width %s) (type solid)) (layer "%s"))\n'
            % (fnum(x0), fnum(y0), fnum(x1), fnum(y1), fnum(width), layer))


def smd_pad(num, x, y, w, h):
    return ('\t(pad "%s" smd rect (at %s %s) (size %s %s) (layers "F.Cu" "F.Paste" "F.Mask"))\n'
            % (num, fnum(x), fnum(y), fnum(w), fnum(h)))


def write(name, body):
    path = os.path.join(OUT, name + ".kicad_mod")
    open(path, "w", encoding="utf-8", newline="\n").write(body + ")\n")
    print("wrote", os.path.normpath(path))


# ---- SI24R1 / HC-24L01-M half-hole module, 12 x 18 mm, 8 castellations at 1.27 mm pitch -------------
# Origin = centre of the module's pad edge. The module body extends toward -y (antenna end at y=-18).
# Pad order viewed from the top with the antenna up, left to right:
#   1 +3.3V  2 GND  3 CE  4 CSN  5 SCK  6 MOSI  7 MISO  8 IRQ        (module spec PDF, page 2-3)
# Carrier pads run 1.0 mm under the module (y -1.0..0) and 1.5 mm outside its edge (0..1.5).
b = header("SI24R1_Module_12x18", "SI24R1 2.4GHz module 12x18mm, 8 half-holes 1.27mm, antenna toward -y")
b += rect(-6, -18, 6, 0, "F.Fab")
b += rect(-6.3, -18.3, 6.3, 1.8, "F.CrtYd", 0.05)
b += line(-6, 0.6, 6, 0.6, "F.SilkS")
for i in range(8):
    b += smd_pad(i + 1, (i - 3.5) * 1.27, 0.25, 0.9, 2.5)
write("SI24R1_Module_12x18", b)

# ---- CR2450 SMT holder MY-2450-01 (LCSC C964845): 36.8 x 23.2 mm -------------------------------
# PCB layout diagram in the vendor drawing: two side pads 6.15 x 6.0 mm, 36.8 mm overall width,
# 9.2 mm from the holder's top edge, 8.0 mm from its bottom edge. Origin = holder centre.
# Which side tab is + and which is - is NOT stated by the drawing text: the board carries 0-ohm
# jumpers (JP1..JP4) so polarity can be chosen after measuring the real part.
b = header("CR2450_Holder_MY-2450-01", "CR2450 SMT holder MY-2450-01, 2 pads, polarity set by 0R jumpers")
b += rect(-18.4, -11.6, 18.4, 11.6, "F.Fab")
b += rect(-18.7, -11.9, 18.7, 11.9, "F.CrtYd", 0.05)
b += rect(-18.4, -11.6, 18.4, 11.6, "F.SilkS", 0.15)
b += ('\t(fp_text user "CR2450" (at 0 0 0) (layer "F.SilkS") %s)\n' % FONT)
for num, x in (("1", -15.325), ("2", 15.325)):
    b += smd_pad(num, x, 0.6, 6.15, 6.0)
write("CR2450_Holder_MY-2450-01", b)

# ---- XKB TS-1187A tact switch 5.1 x 5.1 mm (LCSC C318884) ---------------------------------------
# Land pattern from the vendor drawing: pad centres +-3.0 mm in x, +-1.875 mm in y, pads 1.0 x 0.75 mm.
# Internally A-B are one contact and C-D the other, so the two top pads are net "1", the bottom pads "2".
b = header("SW_TS-1187A", "XKB TS-1187A-x-x-x tact switch 5.1x5.1mm")
b += rect(-2.55, -2.55, 2.55, 2.55, "F.Fab")
b += rect(-3.9, -3.0, 3.9, 3.0, "F.CrtYd", 0.05)
b += rect(-2.55, -2.55, 2.55, 2.55, "F.SilkS", 0.12)
for x, y, n in ((-3.0, -1.875, "1"), (3.0, -1.875, "1"), (-3.0, 1.875, "2"), (3.0, 1.875, "2")):
    b += smd_pad(n, x, y, 1.0, 0.75)
write("SW_TS-1187A", b)
