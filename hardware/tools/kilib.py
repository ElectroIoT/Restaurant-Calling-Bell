"""Tiny KiCad s-expression helpers: read library symbols, find pins."""
import os, re

KI = os.path.expandvars(r"%LOCALAPPDATA%\Programs\KiCad\10.0\share\kicad")

def parse(text):
    tok = re.compile(r'\s*(?:(\()|(\))|"((?:[^"\\]|\\.)*)"|([^\s()"]+))')
    pos, stack, cur = 0, [], []
    while pos < len(text):
        m = tok.match(text, pos)
        if not m:
            break
        pos = m.end()
        if m.group(1):
            stack.append(cur); cur = []
        elif m.group(2):
            done = cur; cur = stack.pop(); cur.append(done)
        elif m.group(3) is not None:
            cur.append(('s', m.group(3)))
        else:
            cur.append(m.group(4))
    return cur[0] if cur else []

def atom(x):
    return x[1] if isinstance(x, tuple) else x

_cache = {}
def load_lib(name):
    if name not in _cache:
        path = os.path.join(KI, "symbols", name + ".kicad_sym")
        _cache[name] = (open(path, encoding="utf-8").read(), None)
    return _cache[name][0]

def find_symbol_text(lib, sym):
    """Return the raw text of one top-level symbol block."""
    text = load_lib(lib)
    key = '(symbol "%s"' % sym
    i = text.find("\n\t" + key)
    if i < 0:
        raise KeyError(f"{lib}:{sym}")
    i += 2
    depth, j, instr = 0, i, False
    while j < len(text):
        c = text[j]
        if c == '"' and text[j-1] != "\\":
            instr = not instr
        elif not instr:
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0:
                    return text[i:j+1]
        j += 1
    raise ValueError("unbalanced")

def symbol_tree(lib, sym):
    return parse(find_symbol_text(lib, sym))

def pins(lib, sym):
    """List of dicts: number, name, x, y, angle, type for every pin of the symbol (all units)."""
    out = []
    def walk(node):
        if not isinstance(node, list):
            return
        if node and node[0] == "pin":
            d = {"type": node[1]}
            for ch in node[3:]:
                if isinstance(ch, list) and ch:
                    if ch[0] == "at":
                        d["x"], d["y"], d["a"] = float(ch[1]), float(ch[2]), float(ch[3])
                    elif ch[0] == "name":
                        d["name"] = atom(ch[1])
                    elif ch[0] == "number":
                        d["num"] = atom(ch[1])
            out.append(d)
            return
        for ch in node:
            walk(ch)
    walk(symbol_tree(lib, sym))
    return out


def get_symbol(lib, sym):
    """Flattened symbol block text (extends resolved), renamed to the given sym name."""
    raw = find_symbol_text(lib, sym)
    m = re.search(r'\(extends "([^"]+)"\)', raw)
    if not m:
        return raw
    parent = m.group(1)
    ptxt = get_symbol(lib, parent)
    out = ptxt.replace('(symbol "%s"' % parent, '(symbol "%s"' % sym, 1)
    out = re.sub(r'\(symbol "%s_(\d+_\d+)"' % re.escape(parent), r'(symbol "%s_\1"' % sym, out)
    cprops = _prop_blocks(raw)
    for name, blk in cprops.items():
        pb = _prop_blocks(out).get(name)
        if pb:
            out = out.replace(pb, blk, 1)
        else:
            i = out.index('(property "Value"')
            out = out[:i] + blk + " " + out[i:]
    return out


def _prop_blocks(text):
    """{property name: full '(property ...)' block text} for top-level properties of one symbol."""
    res, i = {}, 0
    while True:
        i = text.find('(property "', i)
        if i < 0:
            break
        depth, j, instr = 0, i, False
        while j < len(text):
            c = text[j]
            if c == '"' and text[j-1] != "\\":
                instr = not instr
            elif not instr:
                if c == "(":
                    depth += 1
                elif c == ")":
                    depth -= 1
                    if depth == 0:
                        break
            j += 1
        blk = text[i:j+1]
        name = blk.split('"')[1]
        res.setdefault(name, blk)
        i = j + 1
        if '(symbol "' in text[i:i+40]:
            break
    return res


def symbol_pins(lib, sym):
    return _pins_from(parse(get_symbol(lib, sym)))

def _pins_from(tree):
    out = []
    def walk(node):
        if not isinstance(node, list):
            return
        if node and node[0] == "pin":
            d = {"type": node[1]}
            for ch in node[3:]:
                if isinstance(ch, list) and ch:
                    if ch[0] == "at":
                        d["x"], d["y"], d["a"] = float(ch[1]), float(ch[2]), float(ch[3])
                    elif ch[0] == "name":
                        d["name"] = atom(ch[1])
                    elif ch[0] == "number":
                        d["num"] = atom(ch[1])
            out.append(d)
            return
        for ch in node:
            walk(ch)
    walk(tree)
    return out
