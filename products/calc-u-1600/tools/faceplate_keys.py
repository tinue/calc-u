#!/usr/bin/env python3
"""Regenerate the key hit rectangles in app/KeyLayout/ from the layered faceplate SVGs.

Each key in source-material/<model>.svg is a group
`<g id="key-<name>" class="key" data-x=.. data-y=.. data-w=.. data-h=..>` in
the artwork's own pixels. This script rewrites the x, y, w, h of every
`{"<key>", x, y, w, h},` entry in the C++ tables in place, keeping the
tables' order, comments and key names (the named-key vocabulary).

Usage: tools/faceplate_keys.py [--check]
  --check  exit 1 if a table differs from the SVG instead of rewriting it.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# SVG ids that differ from the named-key vocabulary.
SVG_TO_KEY = {
    "lpar": "(", "rpar": ")", "slash": "/", "aster": "*",
    "minus": "-", "dot": ".", "equal": "=", "plus": "+",
}

# (SVG, C++ table). The PC-1500A shares the PC-1500 table; the script checks
# that its SVG has the same rectangles.
TABLES = [
    ("source-material/PC-1500.svg", "app/KeyLayout/PC1500KeyLayout.cpp"),
    ("source-material/PC1600.svg", "app/KeyLayout/PC1600KeyLayout.cpp"),
]
SAME_AS = [("source-material/PC-1500A.svg", "source-material/PC-1500.svg")]

KEY_RE = re.compile(r'<g id="key-([^"]+)" class="key" data-x="([\d.]+)" data-y="([\d.]+)" '
                    r'data-w="([\d.]+)" data-h="([\d.]+)"')
ENTRY_RE = re.compile(r'\{"([^"]+)", [\d.]+, [\d.]+, [\d.]+, [\d.]+\},')


def svg_keys(path):
    """Ordered list of (name, (x, y, w, h)) in the SVG's document order."""
    out = []
    for m in KEY_RE.finditer((ROOT / path).read_text()):
        name = SVG_TO_KEY.get(m.group(1), m.group(1))
        out.append((name, tuple(m.group(i) for i in range(2, 6))))
    return out


def rewrite(svg, cpp):
    # The PC-1600 has two SHIFT keys (svg ids shift2 at the top, shift at
    # the bottom); the table lists them in that order under one name, so
    # repeated names are matched by occurrence, top to bottom.
    pending = {}
    for name, rect in sorted(svg_keys(svg), key=lambda k: (int(k[1][1]), int(k[1][0]))):
        name = "shift" if name == "shift2" else name
        pending.setdefault(name, []).append(rect)
    used = set()

    def entry(m):
        name = m.group(1)
        if name == "lcd":  # the display rectangle is not a key; kept by hand
            return m.group(0)
        if not pending.get(name):
            sys.exit(f"{cpp}: key '{name}' has no rectangle left in {svg}")
        x, y, w, h = pending[name].pop(0)
        used.add(name)
        return f'{{"{name}", {x}, {y}, {w}, {h}}},'

    text = (ROOT / cpp).read_text()
    new = ENTRY_RE.sub(entry, text)
    left = {n: r for n, r in pending.items() if r}
    if left:
        sys.exit(f"{cpp}: keys in {svg} without a table entry: {sorted(left)}")
    return text, new


def main():
    check = "--check" in sys.argv[1:]
    for a, b in SAME_AS:
        if svg_keys(a) != svg_keys(b):
            sys.exit(f"{a} and {b} differ in their key rectangles; they share one table")
    differ = False
    for svg, cpp in TABLES:
        old, new = rewrite(svg, cpp)
        if old != new:
            differ = True
            if check:
                print(f"{cpp} differs from {svg}")
            else:
                (ROOT / cpp).write_text(new)
                print(f"updated {cpp}")
    return 1 if check and differ else 0


if __name__ == "__main__":
    sys.exit(main())
