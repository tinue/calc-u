#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10,<3.15"
# dependencies = ["fonttools", "pyoxipng"]
# ///
"""Generate a layered, semantic SVG of the Sharp PC-1600 front panel.

Coordinate system: 4000 x 1777 units; the key wells, bands, bezel and panels were measured on a
reference drawing of that size.
Key caps, colours and printed details follow a 600 dpi scan of a PC-1600 (not in the repo),
lightened towards the drawing where the scan is too dark.
Keys, filters and legend placement are shared with build_svg_1500.py; fonts, text helper and
output are in tools/faceplate/svgkit.py.

Layers (bottom to top):
  body        brushed aluminium plate
  print-areas display band (LCD + function-key strip), logo panel, grey key panel
  lighting    plate shading, bevelled edges
  print-lines rules and dividers
  print-text  printed labels (shift functions, captions, logo)
  display     LCD bezel, window, LCD background, dots and status segments
  keys        key wells, caps and cap labels

Usage: build_svg_1600.py [--text LINE1 --text LINE2 ...] [--cursor L,C] [--segments RUN,BATT] [--keep-svg] [--no-png] [-o out.svg]
Writes the SVG to -o, the PNG next to it (rendered with headless Chrome, losslessly optimised
with oxipng; the SVG is deleted unless --keep-svg) and <name>-layout.json (key wells, LCD dot
geometry, segment boxes). Without --text, --cursor and --segments the LCD is off: the faceplate.
--text, --cursor and --segments light the LCD for a preview.
"""
import argparse
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True   # keep the source folders free of __pycache__
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "tools" / "faceplate"))

import svgkit as B
import build_svg_1500 as P
from svgkit import FONT_COND, FONT_LOGO, FONT_SANS, emit, end, layer, rich
from lcdfont import FONT

W, H = 4000, 1777

# ---------------------------------------------------------------- colours
ALU = P.ALU
BAND = "#443a31"          # display band and logo panel (scan: black; the drawing's dark brown reads better)
PANEL = "#a6a8aa"         # grey panel behind BS / SHIFT / POWER
BEZEL = "#80858a"         # grey brushed LCD bezel
RED = "#b9433a"           # legends on the plate (scan hue ~5-12 deg: redder than the PC-1500)
ORANGE = "#d4683a"        # legends on the black band (scan hue ~13 deg)
INK = P.INK
WHITE = "#ecebe7"         # print on the band
FN_FACE = "#55585a"       # small function keys on the band
P.LCD_BG = ("#a6b19e", "#9ca895", "#909c8a")   # Calc-U-1600 kP16BackgroundRgb in the middle
LCD_INK = P.LCD_INK
P.CAPS.update({
    "black":  ("#151210", "#2a2622", "#2c2723", "#f4f2ee"),
    "grey16": ("#94969a", "#a7a9ac", "#686a6d", "#101010"),   # number block: darker than the plate
})
P.M = dict(P.MODELS["1500"], cl=("#c8333b", "#d84a50", "#8a1c22", "#ffffff"))  # brighter CL (scan 211/66/75)

# ---------------------------------------------------------------- geometry (PC1600.png)
PLATE = dict(x=4, y=4, w=3992, h=1769, r=50)
P.PLATE = PLATE
BAND_R = dict(x=48, y=63, w=2572, h=850, r=30)          # LCD + function-key strip
LOGO_R = dict(x=2638, y=63, w=1300, h=490)              # logo panel (rule at y 338..346)
LOGO_RULE = (338, 8)
PANEL_R = dict(x=2638, y=553, w=1300, h=260, r=28)      # BS / SHIFT | POWER OFF ON
PANEL_DIV = 3184
BEZEL_R = dict(x=148, y=170, w=2303, h=593, r=14)
WINDOW = dict(x=254, y=211, w=2093, h=519, r=8)
LCD = dict(x=262, y=219, w=2077, h=505)
FN_DIV = [400, 686, 973, 1260, 1546, 1832, 2118]
FN_Y, FN_W, FN_H = 820, 114, 48
FN_X = [241, 487, 773, 1059, 1346, 1632, 1918, 2235]     # DEF, F1..F6, KBII
LEGEND_GAP = P.LEGEND_GAP

# LCD dot matrix: Calc-U-1600 LcdWidget geometry (panel 272,229,2060,487; 156 x 32 dots, 26 x 4 characters)
PANEL_LCD = dict(x=272, y=229, w=2060, h=487)
COLS, ROWS = 156, 32
MX0 = PANEL_LCD["x"] + 0.05 * PANEL_LCD["w"]
MX1 = PANEL_LCD["x"] + 0.955 * PANEL_LCD["w"]
MY0 = PANEL_LCD["y"] + 0.18 * PANEL_LCD["h"]
MY1 = PANEL_LCD["y"] + 1.00 * PANEL_LCD["h"]
DOT_PX, DOT_PY = (MX1 - MX0) / COLS, (MY1 - MY0) / ROWS
DOT = min(DOT_PX, DOT_PY) * 0.9
SEG_FONT = PANEL_LCD["h"] * 26 / 337
SEG_Y = PANEL_LCD["y"] + 0.03 * PANEL_LCD["h"]
# status segments: name -> (raw x measured on a close-up of the display (LcdLayout.hpp), caption, style); remapped like LcdLayout.hpp
_RAW = {"BUSY": 0.04837, "SHIFT": 0.10587, "S": 0.17526, "SMALL": 0.31047, "DE": 0.39611, "RUN": 0.50436,
        "PRO": 0.54401, "RESERVE": 0.60984, "DEF": 0.72086, "ROMAN": 0.76209, "CTRL": 0.81562, "BATT": 0.87192}
SEGS = {"BUSY": ("BUSY", 0), "SHIFT": ("SHIFT", 0), "S": ("S", 1), "SMALL": ("SMALL", 0), "DE": ("DE", 0),
        "G": ("G", 0), "RAD": ("RAD", 0), "RUN": ("RUN", 0), "PRO": ("PRO", 0), "RESERVE": ("RESERVE", 0),
        "DEF": ("DEF", 0), "I": ("I", 0), "II": ("II", 0), "III": ("III", 0), "CTRL": ("CTRL", 0), "BATT": ("BATT", 1)}
SEG_AT = {"G": ("DE", 1.12), "RAD": ("DE", 1.72), "I": ("ROMAN", 0), "II": ("ROMAN", 0), "III": ("ROMAN", 0)}
SEG_BOX = {}


def seg_x(name):
    base, dx = SEG_AT.get(name, (name, 0))
    t = (_RAW[base] - 0.04837) / (0.87192 - 0.04837)
    return PANEL_LCD["x"] + (0.025 + t * (0.9205 - 0.025)) * PANEL_LCD["w"] + dx * SEG_FONT


rrect = P.rrect


# ---------------------------------------------------------------- body & prints
def body_and_prints():
    emit('<g id="brushed-plate" filter="url(#brushed)">')
    layer("body")
    emit(rrect(PLATE, id="plate", fill=ALU))
    end()
    layer("print-areas")
    emit(rrect(BAND_R, id="band", fill=BAND))
    emit(rrect(LOGO_R, id="logo-panel", fill=BAND))
    p = PANEL_R
    emit(f'<path id="key-panel" d="M{p["x"]},{p["y"]} H{p["x"] + p["w"]} V{p["y"] + p["h"] - p["r"]} '
         f'Q{p["x"] + p["w"]},{p["y"] + p["h"]} {p["x"] + p["w"] - p["r"]},{p["y"] + p["h"]} H{p["x"] + p["r"]} '
         f'Q{p["x"]},{p["y"] + p["h"]} {p["x"]},{p["y"] + p["h"] - p["r"]} Z" fill="{PANEL}"/>')
    end()
    emit("</g>")

    layer("lighting")
    emit(rrect(PLATE, fill="url(#aluLight)"))
    for r in (BAND_R, LOGO_R):
        emit(rrect(r, fill="url(#bandShade)"))
        emit(f'<rect x="{r["x"] - 3}" y="{r["y"] - 3}" width="{r["w"] + 6}" height="{r["h"] + 6}" rx="{r.get("r", 0) + 3}" '
             f'fill="none" stroke="#f2f4f6" stroke-opacity="0.8" stroke-width="4"/>')
        emit(rrect(r, fill="none", stroke="#141010", stroke_opacity="0.75", stroke_width="3"))
    emit(f'<g clip-path="url(#plateClip)"><rect x="0" y="1697" width="{W}" height="80" fill="url(#lip)"/></g>')
    emit(f'<rect x="{PLATE["x"] + 2}" y="{PLATE["y"] + 2}" width="{PLATE["w"] - 4}" height="{PLATE["h"] - 4}" '
         f'rx="{PLATE["r"] - 2}" fill="none" stroke="#f1f3f5" stroke-opacity="0.7" stroke-width="3"/>')
    emit(rrect(PLATE, fill="none", stroke=P.ALU_EDGE, stroke_width="3"))
    end()

    layer("print-lines")
    l = LOGO_R
    emit(f'<rect id="logo-rule" x="{l["x"]}" y="{LOGO_RULE[0]}" width="{l["w"]}" height="{LOGO_RULE[1]}" fill="{ALU}"/>')
    emit(f'<rect id="panel-divider" x="{PANEL_DIV - 3}" y="{PANEL_R["y"]}" width="6" height="{PANEL_R["h"]}" '
         f'fill="{ALU}" fill-opacity="0.9"/>')
    divs = " ".join(f"M{x},772 V888" for x in FN_DIV)
    emit(f'<path id="fn-dividers" d="{divs}" stroke="#cfd1d3" stroke-width="4"/>')
    end()


# ---------------------------------------------------------------- keys
def fn_key(ident, x):
    """Small flat key in the function-key strip."""
    y, w, h = FN_Y, FN_W, FN_H
    emit(f'<g id="key-{ident}" class="key" data-x="{x}" data-y="{y}" data-w="{w}" data-h="{h}">')
    emit(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="12" fill="#070606"/>')
    emit(f'<rect x="{x + 8}" y="{y + 7}" width="{w - 16}" height="{h - 14}" rx="5" fill="{FN_FACE}" filter="url(#capFinish)"/>')
    emit(f'<rect x="{x + 9}" y="{y + 7.5}" width="{w - 18}" height="4" rx="2" fill="#ffffff" fill-opacity="0.18"/>')
    emit(f'<rect x="{x + 8}" y="{y + h - 12}" width="{w - 16}" height="5" rx="2" fill="#000" fill-opacity="0.25"/>')
    emit("</g>")


def keys():
    layer("keys")
    key = P.key
    COND = dict(family=FONT_COND, weight=400)
    SANS = dict(family=FONT_SANS, weight=400)
    kw, kh = 178, 130
    col = lambda x0, i: round(x0 + i * 247.3)
    # top-right cluster
    key("bs", 2699, 663, 176, 102, "grey", "BS", size=62, fit=78, **SANS)
    key("shift2", 2946, 663, 176, 102, "orange", "SHIFT", size=62, fit=125, **COND)
    key("off", 3441, 663, 176, 102, "grey", "OFF", size=63, fit=118, **SANS)
    key("on", 3686, 663, 177, 102, "grey", "ON", size=63, fit=87, **SANS)
    # function-key strip
    for ident, x in zip(["def", "f1", "f2", "f3", "f4", "f5", "f6", "kbii"], FN_X):
        fn_key(ident, x)
    # letters
    for i, ch in enumerate("QWERTYUIOP"):
        key(ch.lower(), col(205, i), 956, kw, kh, "black", ch, size=81)
    for i, ch in enumerate("ASDFGHJKL"):
        key(ch.lower(), col(266, i), 1152, kw, kh, "black", ch, size=81)
    key("ctrl", 142, 1349, kw, kh, "grey", "CTRL", size=68, fit=126, **COND)
    for i, ch in enumerate("ZXCVBNM()"):
        ident = {"(": "lpar", ")": "rpar"}.get(ch, ch.lower())
        key(ident, col(390, i), 1349, kw, kh, "black", ch, size=81)
    y, h = 1546, 128
    key("shift", 143, y, kw, h, "orange", "SHIFT", size=68, fit=125, **COND)
    key("sml", 390, y, kw, h, "grey", "SML", size=68, fit=120, **SANS)
    key("rsv", 637, y, kw, h, "grey", ("icon", "updown", 56))
    key("rcl", 885, y, kw, h, "grey", "RCL", size=69, fit=111, **SANS)
    key("space", 1132, y, 426, h, "black", "SPACE", size=66, fit=185, **COND)
    key("down", 1626, y, kw, h, "grey", ("icon", "down", 74))
    key("up", 1873, y, kw, h, "grey", ("icon", "up", 74))
    key("enter", 2120, y, 428, h, "grey", "ENTER", size=66, fit=188, **COND)
    # number block (bigger wells)
    num = [["7", "8", "9", "slash", "cl"], ["4", "5", "6", "aster", "mode"],
           ["1", "2", "3", "minus", "left"], ["0", "dot", "equal", "plus", "right"]]
    for r, row in enumerate(num):
        for i, n in enumerate(row):
            x, y, w, h = NUM_X[i], NUM_Y[r], 194, 144
            if n.isdigit():
                key(n, x, y, w, h, "grey16", n, size=90)
            elif n == "slash":
                key(n, x, y, w, h, "grey16", "/", size=90, fit=42, **SANS)
            elif n == "cl":
                key(n, x, y, w, h, "red", "CL", size=84, fit=109, **SANS)
            elif n == "mode":
                key(n, x, y, w, h, "grey16", "MODE", size=74, fit=141, **COND)
            elif n == "dot":
                key(n, x, y, w, h, "grey16", ("icon", "dot", 46))
            elif n in ("left", "right"):
                key(n, x, y, w, h, "grey16", ("icon", n, 66))
            else:
                key(n, x, y, w, h, "grey16", ("icon", {"aster": "aster", "minus": "bar", "equal": "equal",
                                                         "plus": "plus"}[n], 48))
    end()


NUM_X = [2691, 2939, 3186, 3433, 3680]
NUM_Y = [877, 1093, 1309, 1526]


# ---------------------------------------------------------------- printed text
def above(cx, top, segs, size, color, **kw):
    """Legend centred on cx whose lowest ink sits LEGEND_GAP above `top`."""
    base = top - LEGEND_GAP + P.ink_bottom(segs, size)
    rich(cx, base - 0.35 * size, segs, size, color=color, weight=400, vcap=True, **kw)


def ticks(cx, bottom, height, color, gap=7, width=5):
    emit(f'<path d="M{cx - gap},{bottom - height} V{bottom} M{cx + gap},{bottom - height} V{bottom}" '
         f'stroke="{color}" stroke-width="{width}"/>')


def printed_text():
    layer("print-text")
    # band
    rich(615, 127, "MEMORY SAFE GUARD / AUTO POWER OFF", 56, color=WHITE, weight=400, fit=894, vcap=True,
         family=FONT_COND)
    fn_mid = FN_Y + FN_H / 2
    rich(229, fn_mid, "DEF", 50, color=WHITE, weight=400, anchor="end", vcap=True)
    rich(2223, fn_mid, [("KB", ""), ("II", "")], 50, color=WHITE, weight=400, anchor="end", vcap=True)
    for i, t in enumerate(['!', '"', '#', '$', '%', '&']):
        cx = FN_X[i + 1] + FN_W / 2
        if t == '"':
            ticks(cx, FN_Y - LEGEND_GAP + 4, 26, ORANGE, gap=6, width=5)
        else:
            above(cx, FN_Y + 4, t, 38 if t == "$" else 42, ORANGE)
    above(FN_X[7] + FN_W / 2, FN_Y + 4, "CLICK", 40, ORANGE)
    # logo panel: silver Calc-U logo, white caption
    logo = dict(family=FONT_LOGO, weight=700, fit=825, vcap=True)
    rich(3288, 208, "Calc-U-1600", 160, color="url(#logoLines)", **logo)
    rich(3288, 208, "Calc-U-1600", 160, color="url(#logoSheen)", **logo)
    rich(3288, 208, "Calc-U-1600", 160, color="none", style="stroke:#2b2622;stroke-width:2.6;stroke-linejoin:round",
         **logo)
    rich(3288, 447, "POCKET COMPUTER", 76, color=WHITE, weight=400, fit=655, vcap=True)
    # grey panel
    rich(3311, 663 + 51, "POWER", 53, color=INK, weight=400, vcap=True)
    above(3686 + 177 / 2, 663 + 6, "BREAK", 55, INK)
    # number block legends
    leg = {(0, 0): "\\", (1, 0): "~", (3, 0): "?", (4, 0): "CA", (1, 1): "{", (2, 1): "}", (3, 1): ":",
           (1, 2): "[", (2, 2): "]", (3, 2): ",", (4, 2): "DEL", (2, 3): "@", (3, 3): ";", (4, 3): "INS"}
    for (i, r), t in leg.items():
        size = {"~": 76, ",": 64, "\\": 56, "@": 50}.get(t, 54 if len(t) == 1 else 50)
        above(NUM_X[i] + 97, NUM_Y[r], t, size, RED)
    # drawn legends (as printed on the machine; the font glyphs look different)
    def bottom(r):
        return NUM_Y[r] - LEGEND_GAP
    # ' above 1: wedge, round on top, pointed at the bottom; ` above 4: the same turned by 180 deg (a falling drop)
    cx, b, rw = NUM_X[0] + 97, bottom(2), 5.5
    emit(f'<path id="legend-apostrophe" d="M{cx},{b} L{cx - rw},{b - 22} A{rw},{rw} 0 0 1 {cx + rw},{b - 22} Z" fill="{RED}"/>')
    b = bottom(1)
    emit(f'<path id="legend-grave" d="M{cx},{b - 27.5} L{cx + rw},{b - rw} A{rw},{rw} 0 0 1 {cx - rw},{b - rw} Z" fill="{RED}"/>')
    # broken bar above 0
    b = bottom(3)
    emit(f'<path d="M{cx},{b - 34} V{b - 20} M{cx},{b - 14} V{b}" stroke="{RED}" stroke-width="5"/>')
    # _ above the decimal point: underscore under a hairline box (the space marker), small gap between them
    cx, gap = NUM_X[1] + 97, 5
    emit(f'<g id="legend-underscore"><rect x="{cx - 20}" y="{b - 7}" width="40" height="7" fill="{RED}"/>'
         f'<rect x="{cx - 19}" y="{b - 54}" width="38" height="{47 - gap}" fill="none" stroke="{RED}" '
         f'stroke-width="1.6"/></g>')
    # letter block legends
    above(round(390 + 7 * 247.3) + 89, 1349, "<", 54, RED, fit=46)
    above(round(390 + 8 * 247.3) + 89, 1349, ">", 54, RED, fit=46)
    above(1132 + 213, 1546, "^", 56, RED)
    end()


# ---------------------------------------------------------------- display
def display(lines, cursor, segments):
    layer("display")
    b = BEZEL_R
    emit(f'<rect x="{b["x"] + 4}" y="{b["y"] + 8}" width="{b["w"]}" height="{b["h"]}" rx="{b["r"]}" fill="#000" '
         f'fill-opacity="0.45" filter="url(#soft)"/>')
    emit(rrect(BEZEL_R, id="bezel", fill=BEZEL, filter="url(#brushedCap)"))
    emit(rrect(BEZEL_R, fill="url(#bezelLight)"))
    emit(rrect(BEZEL_R, fill="none", stroke="#1d1d1d", stroke_width="3"))
    emit(f'<rect x="{b["x"] + 3}" y="{b["y"] + 3}" width="{b["w"] - 6}" height="{b["h"] - 6}" rx="{b["r"] - 3}" '
         f'fill="none" stroke="#ffffff" stroke-opacity="0.35" stroke-width="2.5"/>')
    emit(rrect(WINDOW, fill="#1f2a22"))
    emit(rrect(LCD, id="lcd-bg", fill="url(#lcdGrad)"))
    emit(f'<path d="M{LCD["x"]},{LCD["y"]} H{LCD["x"] + LCD["w"]} V{LCD["y"] + 8} H{LCD["x"] + 8} V{LCD["y"] + LCD["h"]} '
         f'H{LCD["x"]} Z" fill="#000" fill-opacity="0.25"/>')
    dots(lines, cursor, segments)
    emit(f'<path d="M{LCD["x"]},{LCD["y"]} H{LCD["x"] + LCD["w"]} V{LCD["y"] + 40} L{LCD["x"]},{LCD["y"] + 90} Z" '
         f'fill="#ffffff" fill-opacity="0.05"/>')
    end()


def dots(lines, cursor, segments):
    emit('<g id="lcd-dots">')
    on, ghost = [], []
    for ln in range(4):
        text = lines[ln] if ln < len(lines) else ""
        for c in range(COLS // 6):
            glyph = FONT.get(text[c] if c < len(text) else " ", FONT["?"]) + ["00000"]
            if cursor == (ln, c):
                glyph = glyph[:7] + ["11111"]
            for ry, row in enumerate(glyph):
                for rx in range(6):
                    bit = row[rx] if rx < 5 else "0"
                    p = (MX0 + (c * 6 + rx + 0.5) * DOT_PX - DOT / 2, MY0 + (ln * 8 + ry + 0.5) * DOT_PY - DOT / 2)
                    (on if bit == "1" else ghost).append(p)

    def rects(pts, dx=0, dy=0):
        return "".join(f"M{x + dx:.2f},{y + dy:.2f}h{DOT:.2f}v{DOT:.2f}h-{DOT:.2f}z" for x, y in pts)
    emit(f'<path id="lcd-ghost" d="{rects(ghost)}" fill="#000" fill-opacity="0.018"/>')
    emit(f'<path id="lcd-dot-shadow" d="{rects(on, 2.5, 3)}" fill="#3d4530" fill-opacity="0.30" filter="url(#lcdshadow)"/>')
    emit(f'<path id="lcd-on" d="{rects(on)}" fill="{LCD_INK}" fill-opacity="0.93"/>')
    emit("</g>")

    emit('<g id="lcd-segments">')
    for name, (txt, inverted) in SEGS.items():
        x = seg_x(name)
        w = len(txt) * SEG_FONT * 0.52
        op = 0.92 if name in segments else 0.0
        SEG_BOX[name] = dict(x=round(x, 1), y=round(SEG_Y, 1), w=round(w, 1), h=round(SEG_FONT, 1))
        emit(f'<g id="seg-{name}" class="segment" opacity="{op}">')
        fill = LCD_INK
        if inverted:  # panel colour in an ink box (S, BATT)
            emit(f'<rect x="{x - 4:.1f}" y="{SEG_Y - 1:.1f}" width="{w + 8:.1f}" height="{SEG_FONT + 2:.1f}" fill="{LCD_INK}"/>')
            fill = P.LCD_BG[1]
        emit(f'<text x="{x:.1f}" y="{SEG_Y + SEG_FONT * 0.82:.1f}" font-family="{FONT_COND}" font-size="{SEG_FONT:.1f}" '
             f'font-weight="700" fill="{fill}" textLength="{w:.1f}" lengthAdjust="spacingAndGlyphs">{txt}</text>')
        emit("</g>")
    emit("</g>")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--text", action="append", help="LCD line (up to 4; repeat the option; default: LCD off)")
    ap.add_argument("--cursor", default="none", help="line,column of the cursor, e.g. 3,13 (default: none)")
    ap.add_argument("--segments", default="", help="lit status segments, e.g. DE,G,RUN,BATT")
    ap.add_argument("--keep-svg", action="store_true", help="keep the SVG next to the PNG")
    ap.add_argument("--no-png", action="store_true", help="write the SVG and layout only")
    ap.add_argument("-o", "--out", required=True, help="SVG path (PNG and layout JSON next to it)")
    a = ap.parse_args()
    out = a.out
    lines = a.text or []
    cursor = None if a.cursor == "none" else tuple(int(v) for v in a.cursor.split(","))
    segments = set(a.segments.split(",")) - {""}
    B.out.clear()
    emit(f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" '
         f'width="{W}" height="{H}" viewBox="0 0 {W} {H}">')
    P.defs()
    body_and_prints()
    printed_text()
    display(lines, cursor, segments)
    keys()
    emit("</svg>")
    svg = "\n".join(B.out)
    keymap = {m[0]: dict(x=float(m[1]), y=float(m[2]), w=float(m[3]), h=float(m[4]))
              for m in re.findall(r'id="key-([^"]+)" class="key" data-x="([^"]+)" data-y="([^"]+)" '
                                  r'data-w="([^"]+)" data-h="([^"]+)"', svg)}
    lcd = dict(PANEL_LCD, cols=COLS, rows=ROWS, matrix_x0=round(MX0, 2), matrix_y0=round(MY0, 2),
               dot_pitch_x=round(DOT_PX, 4), dot_pitch_y=round(DOT_PY, 4), dot=round(DOT, 4))
    B.finish(out, svg, {"size": [W, H], "keys": keymap, "lcd": lcd, "segments": SEG_BOX},
             a.keep_svg, a.no_png)


if __name__ == "__main__":
    main()
