"""One-off: derive stitch.py (rectangular boards) from the NewYearPixelFireworks round-board script."""
src = open(r"D:\My Project\ElectroIoT\NewYearPixelFireworks\tools\stitch.py", encoding="utf-8").read()


def rep(a, b):
    global src
    assert a in src, a[:80]
    src = src.replace(a, b)


rep('"""Add GND stitching vias on a grid wherever they clear all non-GND copper. Run with KiCad\'s python."""',
    '"""Add GND stitching vias on a grid wherever they clear all non-GND copper (rectangular boards).\n'
    'Run with KiCad\'s python:  python.exe stitch.py remote|receiver   (adapted from NewYearPixelFireworks)\n"""')
rep('import math, os\nimport pcbnew', 'import math, os, sys\nimport pcbnew')
rep('H = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "hardware", "kicad")\n'
    'PCB = os.path.join(H, "nypf-base.kicad_pcb")\nCX = CY = 100.0\n',
    'HERE = os.path.dirname(os.path.abspath(__file__))\nsys.path.insert(0, HERE)\n'
    'D = __import__(sys.argv[1] + "_design")\n'
    'PCB = os.path.join(HERE, "..", D.OUT_DIR, D.PROJECT + ".kicad_pcb")\nOX = OY = 100.0\nBW, BH = D.BOARD\n')
rep('RMAX = 55.0\nANT = (CX - 12, CY - 62, CX + 12, CY - 49)  # antenna keep-out box (mm)\n',
    'ANTS = [(OX + a, OY + b, OX + c, OY + d) for (a, b, c, d) in D.KEEPOUTS]\n\n\n'
    'def inside(x, y, extra=0.0):\n    return OX + 1.5 - extra <= x <= OX + BW - 1.5 + extra and OY + 1.5 - extra <= y <= OY + BH - 1.5 + extra\n\n\n'
    'def in_ant(x, y):\n    return any(a[0] - 1 <= x <= a[2] + 1 and a[1] - 1 <= y <= a[3] + 1 for a in ANTS)\n')
rep('k = int(RMAX // pitch)\nfor i in range(-k, k + 1):\n    for j in range(-k, k + 1):\n        x, y = CX + i * pitch, CY + j * pitch\n'
    '        if math.hypot(x - CX, y - CY) > RMAX:\n            continue\n'
    '        if ANT[0] - 1 <= x <= ANT[2] + 1 and ANT[1] - 1 <= y <= ANT[3] + 1:\n            continue\n',
    'for i in range(int(BW // pitch) + 1):\n    for j in range(int(BH // pitch) + 1):\n        x, y = OX + 2.0 + i * pitch, OY + 2.0 + j * pitch\n'
    '        if not inside(x, y) or in_ant(x, y):\n            continue\n')
rep('    if math.hypot(x - CX, y - CY) > RMAX + 2:\n        return False\n'
    '    if ANT[0] - 1 <= x <= ANT[2] + 1 and ANT[1] - 1 <= y <= ANT[3] + 1:\n        return False\n',
    '    if not inside(x, y, 0.5) or in_ant(x, y):\n        return False\n')
open(r"D:\My Project\ElectroIoT\restaurant-calling-bell\hardware\tools\stitch.py", "w", encoding="utf-8").write(src)
print("stitch.py written")
