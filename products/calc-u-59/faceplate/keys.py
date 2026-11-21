#!/usr/bin/env python3
"""Rewrite the key rectangles kKeyRects in app/KeyboardLayout.hpp from the generated faceplate SVGs.

Every key of a faceplate SVG carries its cap rectangle in canvas pixels
(tools/faceplate/svgkit.py svg_keys), the 45 keys row by row as the matrix
(kKeyRects[row][col]). The table holds them normalized to the keyboard section:
x and w to the canvas width, y and h to the section (kKeyboardYStart,
kKeyboardYScale).

Usage: keys.py DIR [--check]   (DIR holds ti59.svg and ti58.svg; build.sh runs it)
  --check  exit 1 if the table differs from the SVG instead of rewriting it.
"""

import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT.parents[1] / "tools" / "faceplate"))
import svgkit  # noqa: E402

HEADER = ROOT / "app" / "KeyboardLayout.hpp"
ROWS, COLS = 9, 5
TABLE_RE = re.compile(r"(inline constexpr NRect kKeyRects\[9\]\[5\] = \{\n)(.*?)(\n\};)", re.S)


def constant(text, name):
    m = re.search(rf"inline constexpr double {name} = ([\d.]+);", text)
    if not m:
        sys.exit(f"{HEADER}: no {name}")
    return float(m.group(1))


def size(svg):
    m = re.search(r'<svg[^>]* width="(\d+)" height="(\d+)"', svg)
    return int(m.group(1)), int(m.group(2))


def table(svg_path, y0, ys):
    svg = svg_path.read_text()
    w, h = size(svg)
    rects = list(svgkit.svg_keys(svg).values())
    if len(rects) != ROWS * COLS:
        sys.exit(f"{svg_path}: {len(rects)} keys, the matrix has {ROWS * COLS}")
    rows = []
    for r in range(ROWS):
        cells = []
        for x, y, cw, ch in rects[r * COLS:(r + 1) * COLS]:
            v = (x / w, (y / h - y0) / ys, cw / w, ch / (h * ys))
            cells.append("{" + ", ".join(f"{c:.6f}" for c in v) + "}")
        rows.append(f"    // row {r}\n    {{{', '.join(cells)}}},")
    return "\n".join(rows)


def main():
    args = [a for a in sys.argv[1:] if a != "--check"]
    if len(args) != 1:
        sys.exit(__doc__)
    check, svg_dir = "--check" in sys.argv[1:], Path(args[0])
    text = HEADER.read_text()
    y0, ys = constant(text, "kKeyboardYStart"), constant(text, "kKeyboardYScale")
    rows = table(svg_dir / "ti59.svg", y0, ys)
    if table(svg_dir / "ti58.svg", y0, ys) != rows:
        sys.exit("ti59.svg and ti58.svg differ in their key rectangles; they share one table")
    new = TABLE_RE.sub(lambda m: m.group(1) + rows + m.group(3), text, count=1)
    if new == text:
        return 0
    if check:
        print(f"{HEADER} differs from the faceplate")
        return 1
    HEADER.write_text(new)
    print(f"updated {HEADER}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
