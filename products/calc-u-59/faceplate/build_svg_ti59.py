#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10,<3.15"
# dependencies = ["fonttools", "pyoxipng"]
# ///
"""Generate the layered, semantic SVG of the TI-59 / TI-58 / TI-58C faceplate of Calc-U-59.

Coordinate system: 2064 x 4075 units, the canvas of Calc-U-59 (app/KeyboardLayout.hpp:
kCanvasAspect, kDisplayRect, kCardRect; kKeyRects are written from the caps here). The layout
is the emulator's, not the original calculator's: key wells, legend rows and the display panel
were measured on a reference rendering of the face, and so were the sizes (one size for all
cap lettering, one for the digits, one for the legends, one math x per context, the signs) and
the colours (ti59-ink.json). Every cap label is centred on its cap; the legends share a
baseline per row, letter-spaced to their width on the target. Type and signs follow a photo
of a TI-58C: Helvetica, a bold italic serif for the math letters. The keys are flat: a cap in a
narrow dark gap, lit from the top.
Fonts, font metrics, the output buffer, rich text and the rendering are in tools/faceplate/svgkit.py.

Layers (bottom to top):
  body        black keyboard panel
  display     dark red glass panel; inside it the LED digits (group led, cells as in
              app/LedGeometry: 12 cells, cell 0 rightmost, modernized segments; empty unless --led)
  print-text  printed legends above the keys (second functions, A'..E')
  keys        flat caps in a black gap, lit from the top (a highlight along each cap's top edge,
              around its top corners and down its sides; a shadow below each well), and their
              labels; data-x/y/w/h = the cap (the app's key rect)
  cue-card    the cue card window (kCardRect), hidden unless --card. The app draws the fitted
              card here and lays a semi-transparent wash (#1A0505, 40 %) over its top 28 %: the
              red display glass the card slides under. Group card holds the card art, card-wash
              the wash.

Usage: build_svg_ti59.py [--model 59|58] [--led 3.141592654] [--card MagnetCard.png]
                         [--legend warm] [--keep-svg] [--no-png] -o out.svg
Writes the SVG to -o, the PNG next to it (rendered with headless Chrome, losslessly optimised
with oxipng; the SVG is deleted unless --keep-svg) and <name>-layout.json (canvas, display, LED cells, card window and wash, key rects).
Without --led and --card: the faceplate (LED off, no card); with them, a preview.
"""
import argparse
import base64
import json
import sys
from pathlib import Path
from xml.sax.saxutils import escape

HERE = Path(__file__).resolve().parent   # faceplate/: the measured data
sys.dont_write_bytecode = True
sys.path.insert(0, str(HERE.parents[2] / "tools" / "faceplate"))
import svgkit  # noqa: E402  (fonts, output buffer, rendering)
from svgkit import emit, end, layer  # noqa: E402
W, H = 2064, 4075
# ink boxes of every legend and cap label on the target
_ink = HERE / "ti59-ink.json"
INK = json.loads(_ink.read_text()) if _ink.exists() else None

# ---------------------------------------------------------------- colours
# Cap faces, labels and legends as measured on the target (ti59-ink.json "colors"),
# a little warmer, towards the TI-58C photo.
PANEL = "#1c1b1a"          # black keyboard panel
GLASS = ("#2a0a09", "#1c0606", "#140303")  # display glass, top -> bottom (the app's background 0x0D0000)
# legend print: a flat colour or a gradient (userSpaceOnUse in every legend's own coordinates:
# baseline at 0, so one gradient runs from the top to the foot of each legend); --legend picks one
LEGENDS_INK = {
    "target": ("#baa57c",),                                   # as measured on the target
    "warm": ("#c9a65e",),                                     # default
    "rich": ("#d4aa45",),
    "card": ("#c49223",),                                     # the app's module cue card gold
    "satin": (("#e8cd85", 0), ("#c49a48", 0.6), ("#a77f34", 1)),
    "metal": (("#b08837", 0), ("#f1dc98", 0.42), ("#c79c45", 0.62), ("#9c7630", 1)),
}
LEGEND, LEGEND_NAME = "#c9a65e", "warm"   # set in main()
CREAM = "#e4d8b8"          # the number keys' "white", a little lighter than on the target
CAPS = {  # face, label; the lettering on the brown caps is the same cream as the number keys
    "brown":  ("#2b2218", CREAM),
    "yellow": ("#e6ae46", "#0e0b08"),
    "cream":  (CREAM, "#0e0b08"),
}
GAP = "#030303"            # the black gap around every cap
LED_ON = "#ff1a1a"         # LedGeometry modernized: active (1.0, 0.1, 0.1)
LED_OFF = "#330000"        # inactive (0.2, 0, 0) at alpha 0.2
WASH, WASH_ALPHA = "#1a0505", 0.4   # app/CueCard.hpp kCueCardWashColor / kCueCardWashAlpha

# ---------------------------------------------------------------- geometry
DISPLAY_PANEL = dict(x=0, y=0, w=W, h=599, r=30)     # dark red glass (measured)
DISPLAY = dict(x=0, y=round(0.052 * H), w=W, h=round(0.05 * H))   # kDisplayRect: LED cells
CARD = dict(x=0, y=round(0.11 * H), w=W, h=440)      # kCardRect (card art is 2064 x 440)
WASH_H = round(CARD["h"] * 0.28)                      # CueCardGeometry::washHeight
CELLS, SLANT = 12, 0.14

# key wells measured on ti59_rendered.png; caps inset so they cover the app's key rects
COL_X, COL_W = [14, 448, 884, 1319, 1752], 296
DIG_X, DIG_W = [400, 850, 1300], 367
ROW_Y = [1048, 1399, 1749, 2099, 2448, 2798, 3152, 3504, 3855]
ROW_H = [195] * 5 + [199] * 4
RIM = 14                   # dark gap around the flat cap; the cap covers the app's key rect
# Light from the top, as on the reference: a highlight along the top edge of every cap that
# wraps around its top corners and runs a little way down both sides, thinning out (white over
# the face, strongest on the light cream caps, where less would not show), and a soft shadow
# each key throws below its well, which thickens the dark rim under the cap.
LIGHT_BAND, LIGHT_SIDE, LIGHT_TAIL = 13, 4, 40   # band height, side width, how far the sides run down
LIGHT = {"brown": 0.30, "yellow": 0.45, "cream": 0.72}   # white opacity per cap colour
CAP_R = 12                 # corner radius of a cap
SHADOW_DROP, SHADOW_BLUR, SHADOW_ALPHA = 7, 4, 0.8
LEGEND_GAP = 33            # baseline of a legend -> top of its well (median on the target)
CAP_SIZE = 115             # all cap lettering: cap height 84 as on the target
CAP_CONDENSE = 0.95        # Helvetica bold is a little wider than the target's lettering
CAP_MARGIN = 18            # minimum space left and right of a label on its cap
DIGIT_SIZE = 134           # digits: height 98 as on the target
LS, LSTROKE = 88, 3.0      # all legends: Helvetica regular + outline ~ medium weight, cap height 67

# ---------------------------------------------------------------- fonts
FONTS = {  # key: the embedded font (svgkit.EMBED: family, file, weight, style)
    "b": svgkit.face("PCHeros", 700),
    "r": svgkit.face("PCHeros", 400),
    "m": svgkit.face("PCTermes", 700),
}


def advance(text, key, size):
    return svgkit.advance(text, FONTS[key][1], size)


def glyph_runs(text, key, size):
    return svgkit.glyph_runs(text, FONTS[key][1], size)


# ---------------------------------------------------------------- rich text
# A label is a list of runs. A run is (text, font key[, scale, rise]) or a drawn glyph:
#   ("X", ch, scale, stretch, stroke[, rise])  math letter: bold italic serif, stretched
#               horizontally by `stretch`, outlined by `stroke` (in its own size)
#   ("prime",)  slanted prime after the previous run
#   ("wedge",)  the x<->t exchange sign (two filled half arrows)
#   ("arrow",)  right arrow at the middle of the cap height
#   ("plus",)   plus sign centred on a 0.85 Sigma (Sigma+)
#   ("pi",)     italic pi with an even stroke
#   ("geq",)    greater-or-equal: chevron over a bar, ("eq",) a wide equals sign
#   ("vbar",)   vertical bar from the baseline to the cap height (|x|)
#   ("slash",)  the long, flat fraction slash of 1/x
#   ("root",)   radical sign, drawn with the vinculum over the next run (as printed: round, heavy)
#   ("bar",)    overline over the next run
#   ("gap", f)  f label sizes of space
# scale is relative to the label size, rise in label sizes (positive = up).
# `track` is extra space after every glyph or drawn sign but the last, in units.
# Drawn signs: (advance, ink box x0, y0, x1, y1) in label sizes, SVG y (down).
SIGNS = {"prime": (0.16, 0.01, -0.74, 0.18, -0.46), "wedge": (0.62, 0.10, -0.70, 0.62, -0.03),
         "arrow": (0.88, 0.06, -0.52, 0.82, -0.20), "plus": (0.56, 0.05, -0.52, 0.51, -0.06),
         "pi": (0.84, 0.0, -0.68, 0.82, 0.0), "root": (0.50, -0.035, -0.785, 0.525, 0.0),
         "geq": (0.56, 0.03, -0.72, 0.55, -0.02), "eq": (0.78, 0.03, -0.47, 0.75, -0.12),
         "vbar": (0.12, 0.0, -0.72, 0.12, 0.0), "slash": (0.44, 0.0, -0.76, 0.60, 0.10)}


def label_runs(runs, size, track=0.0):
    """Lay out runs from x=0 on the baseline; returns (items, width)."""
    items, x, pending = [], 0.0, None
    for run in runs:
        kind = run[0]
        if kind in SIGNS:
            items.append((kind, x, size))
            x += SIGNS[kind][0] * size + track
            if kind == "root":
                pending = items[-1]
            continue
        if kind == "gap":
            x += run[1] * size
            continue
        if kind == "bar":
            pending = "bar"
            continue
        if kind == "X":
            ch, sc, stretch, stroke = run[1:5]
            rise = run[5] if len(run) > 5 else 0.0
            s = size * sc
            w = advance(ch, "m", s) * stretch
            items.append(("X", x, ch, s, stretch, stroke * s, rise * size))
        else:
            text, key = run[0], run[1]
            s = size * (run[2] if len(run) > 2 else 1.0)
            w = advance(text, key, s) + track * len(text)
            items.append(("text", x, text, key, s, (run[3] if len(run) > 3 else 0.0) * size, track))
        if pending:
            if pending == "bar":
                items.append(("over", items[-1], size))
            else:
                items.append(("vinc", items[-1], size, pending[1]))
            pending = None
        x += w + (track if kind == "X" else 0.0)
    return items, x - track


def item_ink(it):
    """Ink box (x0, y0, x1, y1) of one laid-out item, or None."""
    k, x = it[0], it[1]
    if k == "text":
        _, x, text, key, s, rise, track = it
        bx = [(x + dx + i * track + b[0], -rise - b[3], x + dx + i * track + b[2], -rise - b[1])
              for i, (dx, b) in enumerate(glyph_runs(text, key, s)) if b]
        return (min(b[0] for b in bx), min(b[1] for b in bx), max(b[2] for b in bx), max(b[3] for b in bx)) if bx else None
    if k == "X":
        _, x, ch, s, stretch, stroke, rise = it
        b = glyph_runs(ch, "m", s)[0][1]
        e = stroke / 2
        return (x + b[0] * stretch - e, -rise - b[3] - e, x + b[2] * stretch + e, -rise - b[1] + e)
    if k in SIGNS:
        s = it[2]
        _, x0, y0, x1, y1 = SIGNS[k]
        return (x + x0 * s, y0 * s, x + x1 * s, y1 * s)
    if k == "vinc":   # the radical: hook, rise and the bar 0.2 past the end of the radicand
        b, s, rx = item_ink(it[1]), it[2], it[3]
        return (rx - 0.035 * s, -0.785 * s, b[2] + 0.265 * s, 0.0)
    if k == "over":   # overline just above the letter
        b, s = item_ink(it[1]), it[2]
        y = b[1] - 0.10 * s
        return (b[0] + 0.08 * (b[2] - b[0]), y - 0.0425 * s, b[2], y + 0.0425 * s)
    return None


def ink_box(items, stroke):
    """Ink bounds (x0, y0, x1, y1) of laid-out items, SVG y (down), relative to the baseline origin."""
    bx = [b for b in map(item_ink, items) if b]
    h = stroke / 2
    return (min(b[0] for b in bx) - h, min(b[1] for b in bx) - h, max(b[2] for b in bx) + h, max(b[3] for b in bx) + h)


def draw_label(runs, size, color, stroke=0.0, cx=None, cy=None, base=None, maxw=None, condense=1.0, track=0.0):
    """Centre the ink of the runs horizontally at cx and either vertically at cy or with the
    baseline at `base`; condensed by `condense`, and further if wider than maxw."""
    items, _ = label_runs(runs, size, track)
    x0, y0, x1, y1 = ink_box(items, stroke)
    sx = condense
    if maxw and (x1 - x0) * sx > maxw:
        sx = maxw / (x1 - x0)
    ty = base if base is not None else cy - (y0 + y1) / 2
    emit(f'<g transform="translate({cx - (x0 + x1) / 2 * sx:.1f},{ty:.1f}) scale({sx:.4f},1)">')
    st = (f' stroke="{color}" stroke-width="{stroke:.1f}" stroke-linejoin="round"' if stroke else "")
    sw = 0.115 * size   # stroke of the drawn signs
    for it in items:
        k = it[0]
        if k == "text":
            _, x, text, key, s, rise, tr = it
            fam, _, wt, sty = FONTS[key]
            ls = f' letter-spacing="{tr:.1f}"' if tr else ""
            emit(f'<text x="{x:.1f}" y="{-rise:.1f}" font-family="{fam}" font-weight="{wt}" font-style="{sty}" '
                 f'font-size="{s:.1f}" fill="{color}"{ls}{st}>{escape(text)}</text>')
        elif k == "X":
            _, x, ch, s, stretch, xs, rise = it
            fam, _, wt, sty = FONTS["m"]
            emit(f'<text transform="translate({x:.1f},{-rise:.1f}) scale({stretch},1)" font-family="{fam}" '
                 f'font-weight="{wt}" font-style="{sty}" font-size="{s:.1f}" fill="{color}" stroke="{color}" '
                 f'stroke-width="{xs:.1f}" stroke-linejoin="round">{escape(ch)}</text>')
        elif k in SIGNS:
            x, s = it[1], it[2]
            P = lambda fx, fy: f"{x + fx * s:.1f},{fy * s:.1f}"
            if k == "prime":
                emit(f'<path d="M{P(0.10, -0.74)} L{P(0.18, -0.74)} L{P(0.06, -0.46)} L{P(0.01, -0.46)} Z" fill="{color}"/>')
            elif k == "wedge":
                emit(f'<path d="M{P(0.10, -0.70)} L{P(0.56, -0.53)} L{P(0.10, -0.37)} Z '
                     f'M{P(0.62, -0.37)} L{P(0.16, -0.20)} L{P(0.62, -0.03)} Z" fill="{color}"/>')
            elif k == "arrow":
                emit(f'<path d="M{P(0.06, -0.398)} H{x + 0.58 * s:.1f} V{-0.52 * s:.1f} L{P(0.82, -0.36)} '
                     f'L{P(0.58, -0.20)} V{-0.322 * s:.1f} H{x + 0.06 * s:.1f} Z" fill="{color}"/>')
            elif k == "plus":   # centred on a Sigma of 0.85 size: 0.85 * 0.729 / 2
                c, a = -0.31, 0.23
                emit(f'<path d="M{P(0.05, c)} H{x + 0.51 * s:.1f} M{P(0.28, c - a)} V{(c + a) * s:.1f}" '
                     f'stroke="{color}" stroke-width="{sw:.1f}"/>')
            elif k == "pi":     # even stroke: bar with a curled start, a hooked left leg, a straight right leg
                h, e = sw / 2, sw / 2 / s
                emit(f'<path d="M{P(0.0 + e, -0.55)} Q{P(0.08, -0.64 + e)} {P(0.22, -0.64 + e)} H{x + 0.82 * s - h:.1f} '
                     f'M{P(0.33, -0.64)} Q{P(0.28, -0.22)} {P(0.10, -e)} '
                     f'M{P(0.62, -0.64)} Q{P(0.55, -0.24)} {P(0.59, -0.08)} Q{P(0.62, 0.0 - e)} {P(0.71, -0.05)}" '
                     f'fill="none" stroke="{color}" stroke-width="{sw:.1f}" stroke-linecap="round"/>')
            elif k == "geq":
                emit(f'<path d="M{P(0.06, -0.69)} L{P(0.50, -0.47)} L{P(0.06, -0.25)} M{P(0.03, -0.065)} H{x + 0.55 * s:.1f}" '
                     f'fill="none" stroke="{color}" stroke-width="{0.09 * s:.1f}" stroke-linejoin="miter"/>')
            elif k == "eq":
                emit(f'<path d="M{P(0.03, -0.425)} H{x + 0.75 * s:.1f} M{P(0.03, -0.165)} H{x + 0.75 * s:.1f}" '
                     f'stroke="{color}" stroke-width="{0.09 * s:.1f}"/>')
            elif k == "vbar":
                emit(f'<rect x="{x:.1f}" y="{-0.72 * s:.1f}" width="{0.12 * s:.1f}" height="{0.72 * s:.1f}" fill="{color}"/>')
            elif k == "slash":
                emit(f'<path d="M{P(0.06, 0.06)} L{P(0.54, -0.72)}" stroke="{color}" stroke-width="{0.125 * s:.1f}" '
                     f'stroke-linecap="square"/>')
            # "root" draws nothing itself: its "vinc" draws the whole radical
        elif k == "vinc":
            rx, s, x1 = it[3], it[2], item_ink(it[1])[2]
            emit(f'<path d="M{rx + 0.03 * s:.1f},{-0.37 * s:.1f} L{rx + 0.14 * s:.1f},{-0.065 * s:.1f} '
                 f'L{rx + 0.46 * s:.1f},{-0.72 * s:.1f} H{x1 + 0.20 * s:.1f}" fill="none" stroke="{color}" '
                 f'stroke-width="{0.13 * s:.1f}" stroke-linecap="round" stroke-linejoin="round"/>')
        elif k == "over":
            x0b, y0b, x1b, y1b = item_ink(it)
            emit(f'<rect x="{x0b:.1f}" y="{y0b:.1f}" width="{x1b - x0b:.1f}" height="{y1b - y0b:.1f}" fill="{color}"/>')
    emit("</g>")


# ---------------------------------------------------------------- defs
def defs():
    emit("<defs>")
    emit(f"<style>{svgkit.font_faces(('PCHeros', 'PCTermes'))}</style>")
    emit(f"""
<filter id="ledGlow" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur stdDeviation="{0.8 * seg_width() / 2:.1f}"/></filter>
<filter id="ledBloom" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur stdDeviation="{0.2 * seg_width() / 2:.1f}"/></filter>
<filter id="keyShadow" x="-10%" y="-20%" width="120%" height="140%"><feGaussianBlur stdDeviation="{SHADOW_BLUR}"/></filter>
<linearGradient id="glassGrad" x1="0" y1="0" x2="0" y2="1">
  <stop offset="0" stop-color="{GLASS[0]}"/>
  <stop offset="0.5" stop-color="{GLASS[1]}"/>
  <stop offset="1" stop-color="{GLASS[2]}"/>
</linearGradient>
<linearGradient id="glassSheen" x1="0" y1="0" x2="0.25" y2="1">
  <stop offset="0" stop-color="#ffffff" stop-opacity="0.06"/>
  <stop offset="0.35" stop-color="#ffffff" stop-opacity="0.015"/>
  <stop offset="0.36" stop-color="#ffffff" stop-opacity="0"/>
  <stop offset="1" stop-color="#000000" stop-opacity="0"/>
</linearGradient>
{legend_gradient()}
<clipPath id="cardClip"><rect x="{CARD['x']}" y="{CARD['y']}" width="{CARD['w']}" height="{CARD['h']}"/></clipPath>
""")
    emit("</defs>")


# ---------------------------------------------------------------- body and display
def legend_gradient():
    ink = LEGENDS_INK[LEGEND_NAME]
    if len(ink) == 1:
        return ""
    stops = "".join(f'<stop offset="{o}" stop-color="{c}"/>' for c, o in ink)
    return (f'<linearGradient id="legendInk" gradientUnits="userSpaceOnUse" x1="0" y1="{-0.78 * LS:.1f}" x2="0" '
            f'y2="{0.05 * LS:.1f}">{stops}</linearGradient>')


def body():
    layer("body")
    emit(f'<rect id="panel" x="0" y="0" width="{W}" height="{H}" fill="{PANEL}"/>')
    end()


def panel_path(r):
    """The display panel: square at the top (canvas edge), rounded at the bottom."""
    x, y, w, h, rr = r["x"], r["y"], r["w"], r["h"], r["r"]
    return (f"M{x},{y} H{x + w} V{y + h - rr} Q{x + w},{y + h} {x + w - rr},{y + h} "
            f"H{x + rr} Q{x},{y + h} {x},{y + h - rr} Z")


def seg_width():
    w = DISPLAY["w"] / CELLS
    return (w - 2 * w * 0.22) * 0.14


def led_segments(w, h):
    """LedGeometry::modernizedSegments: hexagonal segments A..G in one cell."""
    px, py = w * 0.22, h * 0.10
    rw, rh = w - 2 * px, h - 2 * py
    sw = rw * 0.14
    gap = sw * 0.15
    hh, maxx, maxy, midy = rh / 2, px + rw, py + rh, py + rh / 2
    hl, vl = rw - sw - 2 * gap, hh - sw - 2 * gap

    def hs(x, y, n):
        c = sw / 2
        return [(x + c, y), (x + n - c, y), (x + n, y + sw / 2), (x + n - c, y + sw), (x + c, y + sw), (x, y + sw / 2)]

    def vs(x, y, n):
        c = sw / 2
        return [(x + sw / 2, y), (x + sw, y + c), (x + sw, y + n - c), (x + sw / 2, y + n), (x, y + n - c), (x, y + c)]
    return [hs(px + sw / 2 + gap, py, hl), vs(maxx - sw, py + sw / 2 + gap, vl), vs(maxx - sw, midy + sw / 2 + gap, vl),
            hs(px + sw / 2 + gap, maxy - sw, hl), vs(px, midy + sw / 2 + gap, vl), vs(px, py + sw / 2 + gap, vl),
            hs(px + sw / 2 + gap, midy - sw / 2, hl)]


GLYPHS = {"0": "ABCDEF", "1": "BC", "2": "ABDEG", "3": "ABCDG", "4": "BCFG", "5": "ACDFG", "6": "ACDEFG",
          "7": "ABC", "8": "ABCDEFG", "9": "ABCDFG", "-": "G", "E": "ADEFG", "C": "ADEF", " ": ""}


def cells_from_text(text):
    """Right-aligned: each character a cell, a '.' lights the decimal point of the cell before it."""
    cells = []
    for ch in text:
        if ch == "." and cells:
            cells[-1] = (cells[-1][0], True)
        else:
            cells.append((ch, False))
    cells = cells[-CELLS:]
    return [(" ", False)] * (CELLS - len(cells)) + cells   # left to right


def display(text):
    layer("display")
    emit(f'<path id="display-glass" d="{panel_path(DISPLAY_PANEL)}" fill="url(#glassGrad)"/>')
    emit(f'<path d="{panel_path(DISPLAY_PANEL)}" fill="url(#glassSheen)"/>')
    emit(f'<path d="M0,{DISPLAY_PANEL["h"] - DISPLAY_PANEL["r"]} Q0,{DISPLAY_PANEL["h"]} {DISPLAY_PANEL["r"]},{DISPLAY_PANEL["h"]} '
         f'H{W - DISPLAY_PANEL["r"]} Q{W},{DISPLAY_PANEL["h"]} {W},{DISPLAY_PANEL["h"] - DISPLAY_PANEL["r"]}" fill="none" '
         f'stroke="#000" stroke-opacity="0.55" stroke-width="4"/>')
    cw, ch = DISPLAY["w"] / CELLS, DISPLAY["h"]
    segs = led_segments(cw, ch)
    emit('<g id="led">')
    lit, unlit, dots = [], [], []
    for col, (c, dp) in enumerate(cells_from_text(text) if text else []):
        x0, y0 = DISPLAY["x"] + col * cw, DISPLAY["y"]
        on = GLYPHS.get(c, "")
        for i, poly in enumerate(segs):
            pts = " ".join(f"{x0 + x + SLANT * (ch - y):.1f},{y0 + y:.1f}" for x, y in poly)
            (lit if "ABCDEFG"[i] in on else unlit).append(f"M{pts}Z")
        if dp:
            s = ch * 0.13
            dots.append((x0 + cw - s * 1.1 + s / 2, y0 + ch - s * 1.4 + s / 2, s / 2))
    if text:
        emit(f'<path id="led-off" d="{"".join(unlit)}" fill="{LED_OFF}" fill-opacity="0.2"/>')
        dp = "".join(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r:.1f}"/>' for x, y, r in dots)
        emit(f'<g id="led-glow" fill="{LED_ON}" opacity="0.6" filter="url(#ledGlow)"><path d="{"".join(lit)}"/>{dp}</g>')
        emit(f'<g id="led-bloom" fill="{LED_ON}" filter="url(#ledBloom)"><path d="{"".join(lit)}"/>{dp}</g>')
        emit(f'<g id="led-on" fill="{LED_ON}"><path d="{"".join(lit)}"/>{dp}</g>')
    emit("</g>")
    end()


# ---------------------------------------------------------------- keys
B, R = "b", "r"   # Helvetica bold, Helvetica regular


def key(ident, x, y, w, h, scheme, runs, op_box=None):
    """A key lit from the top: its shadow below the well, the dark gap around the cap (where the
    real one shows the PCB), the flat cap with a highlight band along its top edge, and its
    label, centred on the cap."""
    face, lcol = CAPS[scheme]
    cx, cy, cw, ch = x + RIM, y + RIM, w - 2 * RIM, h - 2 * RIM
    emit(f'<g id="key-{ident}" class="key" data-x="{cx}" data-y="{cy}" data-w="{cw}" data-h="{ch}">')
    emit(f'<rect x="{x}" y="{y + SHADOW_DROP}" width="{w}" height="{h}" rx="24" fill="#000000" '
         f'fill-opacity="{SHADOW_ALPHA}" filter="url(#keyShadow)"/>')
    emit(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="24" fill="{GAP}"/>')
    emit(f'<rect x="{cx}" y="{cy}" width="{cw}" height="{ch}" rx="{CAP_R}" fill="{face}"/>')
    emit(f'<path d="{highlight(cx, cy, cw)}" fill="#ffffff" fill-opacity="{LIGHT[scheme]}"/>')
    mx, my = cx + cw / 2, cy + ch / 2
    if isinstance(runs, str):
        op_icon(runs, mx, my, op_box, lcol)
    else:
        size = DIGIT_SIZE if scheme == "cream" else CAP_SIZE
        draw_label(runs, size, lcol, cx=mx, cy=my, maxw=cw - 2 * CAP_MARGIN, condense=CAP_CONDENSE)
    emit("</g>")


def highlight(cx, cy, cw):
    """The highlight of a cap: its outline from the left tail up around the top corners to the
    right tail, back along an inner edge that leaves the sides at LIGHT_SIDE and the top band
    at LIGHT_BAND, both thinning out towards the tails."""
    x1, r, b, sd, t = cx + cw, CAP_R, LIGHT_BAND, LIGHT_SIDE, LIGHT_TAIL
    return (f"M{cx},{cy + t} V{cy + r} Q{cx},{cy} {cx + r},{cy} H{x1 - r} Q{x1},{cy} {x1},{cy + r} V{cy + t} "
            f"Q{x1 - sd},{cy + b + 2} {x1 - sd - r},{cy + b} H{cx + sd + r} "
            f"Q{cx + sd},{cy + b + 2} {cx},{cy + t} Z")


def op_icon(kind, mx, my, size, color):
    """Operator signs drawn as paths, centred at mx, my; size = (w, h) of the sign as on the target."""
    w, h = size
    x, y = mx - w / 2, my - h / 2
    t = 17.0   # stroke, as printed on the target
    if kind == "div":
        r = t * 0.95
        emit(f'<rect x="{x:.1f}" y="{my - t / 2:.1f}" width="{w:.1f}" height="{t:.1f}" fill="{color}"/>')
        for yy in (y + r, y + h - r):
            emit(f'<circle cx="{mx:.1f}" cy="{yy:.1f}" r="{r:.1f}" fill="{color}"/>')
    elif kind == "mul":
        d = t / 2 / 1.414
        emit(f'<path d="M{x + d:.1f},{y + d:.1f} L{x + w - d:.1f},{y + h - d:.1f} M{x + w - d:.1f},{y + d:.1f} L{x + d:.1f},{y + h - d:.1f}" '
             f'stroke="{color}" stroke-width="{t:.1f}"/>')
    elif kind == "minus":
        emit(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" fill="{color}"/>')
    elif kind == "plus":
        emit(f'<path d="M{x:.1f},{my:.1f} H{x + w:.1f} M{mx:.1f},{y:.1f} V{y + h:.1f}" stroke="{color}" stroke-width="{t:.1f}"/>')
    elif kind == "equal":
        emit(f'<path d="M{x:.1f},{y + t / 2:.1f} H{x + w:.1f} M{x:.1f},{y + h - t / 2:.1f} H{x + w:.1f}" '
             f'stroke="{color}" stroke-width="{t:.1f}"/>')
    elif kind == "dot":
        emit(f'<circle cx="{mx:.1f}" cy="{my:.1f}" r="{min(w, h) / 2:.1f}" fill="{color}"/>')
    elif kind == "chs":   # + / -: plus at the top left, minus (as wide as the plus) at the lower right
        a = 0.36 * w
        emit(f'<path d="M{x:.1f},{y + a / 2:.1f} H{x + a:.1f} M{x + a / 2:.1f},{y:.1f} V{y + a:.1f} '
             f'M{x + 0.37 * w:.1f},{y + h:.1f} L{x + 0.62 * w:.1f},{y:.1f} '
             f'M{x + w - a:.1f},{y + h - a / 2:.1f} H{x + w:.1f}" stroke="{color}" stroke-width="{t:.1f}"/>')
    else:
        raise ValueError(kind)


# operator signs: (w, h) as measured on the target
OP_SIZE = {"div": (100, 97), "mul": (92, 92), "minus": (99, 17), "plus": (101, 97), "equal": (113, 52),
           "dot": (36, 36), "chs": (189, 108)}

def math_letter(ch, w, h, base, stroke=0.05, rise=0.0):
    """A math letter (bold italic serif, outlined by `stroke` of its size) whose ink is w x h
    units, stretched horizontally to that width; as a run of a label of size `base`."""
    _, b = glyph_runs(ch, "m", 1000.0)[0]
    gw, gh = (b[2] - b[0]) / 1000, (b[3] - b[1]) / 1000
    s = h / (gh + stroke)
    return ("X", ch, s / base, (w - stroke * s) / (gw * s), stroke, rise)


# The math letters as on the target (ink w x h measured there), the same glyph everywhere:
# XC on the caps (x^2, sqrt x, ln x, x<->t, 1/x), YC and the exponent XS of y^x,
# XL in the legends (x = t, x >= t, |x|, x bar).
XC = math_letter("x", 64, 59, CAP_SIZE, stroke=0.03)   # a little lighter and smaller than the target's
YC = math_letter("y", 76, 84, CAP_SIZE)
XS = math_letter("x", 44, 40, CAP_SIZE, rise=0.45)
XD = XC[:5] + (-0.06,)   # XC lowered to the foot of the 1/x slash
XN = XC[:5] + (0.035,)   # XC after "ln": lifted 4 units, as it looks lower than the n otherwise
XL = math_letter("x", 53, 48, LS, stroke=0.035)


# cap labels per row, columns 0..4: (cap scheme, runs or an operator sign)
def _b(t):
    return ("brown", [(t, B)])


def _d(t):
    return ("cream", [(t, B)])


LABELS = [
    [_b("A"), _b("B"), _b("C"), _b("D"), _b("E")],
    [("yellow", [("2nd", B)]), _b("INV"), ("brown", [("ln", B), ("gap", 0.09), XN]), _b("CE"), ("yellow", [("CLR", B)])],
    [_b("LRN"), ("brown", [XC, ("wedge",), ("gap", 0.11), ("t", B)]), ("brown", [XC, ("gap", 0.04), ("2", R, 0.62, 0.34)]),
     ("brown", [("root",), XC]), ("brown", [("1", B, 0.78, 0.17), ("gap", -0.12), ("slash",), ("gap", 0.05), XD])],
    [_b("SST"), _b("STO"), _b("RCL"), _b("SUM"), ("brown", [YC, ("gap", 0.05), XS])],
    [_b("BST"), _b("EE"), _b("("), _b(")"), ("yellow", "div")],
    [_b("GTO"), _d("7"), _d("8"), _d("9"), ("yellow", "mul")],
    [_b("SBR"), _d("4"), _d("5"), _d("6"), ("yellow", "minus")],
    [_b("RST"), _d("1"), _d("2"), _d("3"), ("yellow", "plus")],
    [_b("R/S"), _d("0"), ("cream", "dot"), ("cream", "chs"), ("yellow", "equal")],
]
IDS = [["A", "B", "C", "D", "E"], ["2nd", "INV", "lnx", "CE", "CLR"], ["LRN", "xt", "x2", "sqrt", "1x"],
       ["SST", "STO", "RCL", "SUM", "yx"], ["BST", "EE", "lpar", "rpar", "div"], ["GTO", "7", "8", "9", "mul"],
       ["SBR", "4", "5", "6", "minus"], ["RST", "1", "2", "3", "plus"], ["RS", "0", "dot", "chs", "equal"]]


def well(r, c):
    if r >= 5 and 1 <= c <= 3:
        return DIG_X[c - 1], ROW_Y[r], DIG_W, ROW_H[r]
    return COL_X[c], ROW_Y[r], COL_W, ROW_H[r]


def keys():
    layer("keys")
    for r, row in enumerate(LABELS):
        for c, (scheme, runs) in enumerate(row):
            x, y, w, h = well(r, c)
            key(IDS[r][c], x, y, w, h, scheme, runs, OP_SIZE.get(runs) if isinstance(runs, str) else None)
    end()


# ---------------------------------------------------------------- printed legends
# One size for all legends: Helvetica regular + an outline ~ the target's medium weight, cap height
# 67 units; the baseline LEGEND_GAP above the well. A little letter spacing widens a legend
# towards its width on the target (ti59-ink.json).
LEGENDS = [
    [[("A", R), ("prime",)], [("B", R), ("prime",)], [("C", R), ("prime",)], [("D", R), ("prime",)], [("E", R), ("prime",)]],
    [None, None, [("log", R)], [("CP", R)], None],
    [[("Pgm", R)], [("P", R), ("arrow",), ("R", R)], [("sin", R)], [("cos", R)], [("tan", R)]],
    [[("Ins", R)], [("CMs", R)], [("Exc", R)], [("Prd", R)], [("Ind", R)]],
    [[("Del", R)], [("Eng", R)], [("Fix", R)], [("Int", R)], [("vbar",), ("gap", 0.18), XL, ("gap", 0.18), ("vbar",)]],
    [[("Pause", R)], [XL, ("gap", 0.14), ("eq",), ("gap", 0.10), ("t", R)], [("Nop", R)], [("Op", R)], [("Deg", R)]],
    [[("Lbl", R)], [XL, ("gap", 0.14), ("geq",), ("gap", 0.14), ("t", R)], [("Σ", R, 0.85), ("gap", 0.08), ("plus",)],
     [("bar",), XL], [("Rad", R)]],
    [[("St flg", R)], [("If flg", R)], [("D.MS", R)], [("pi",)], [("Grad", R)]],
    [[("Write", R)], [("Dsz", R)], [("Adv", R)], [("Prt", R)], [("List", R)]],
]


def legend_track(runs, target_w):
    """Letter spacing towards the legend's width on the target (0 if narrower, at most 0.05 LS:
    the target's letters are wider, not spaced much further apart)."""
    items, _ = label_runs(runs, LS)
    x0, _, x1, _ = ink_box(items, LSTROKE)
    n = sum(len(it[2]) if it[0] == "text" else 1 for it in items if it[0] in ("text", "X") or it[0] in SIGNS)
    return max(0.0, min(0.05 * LS, (target_w - (x1 - x0)) / max(1, n - 1)))


def printed_text(model):
    layer("print-text")
    for r, row in enumerate(LEGENDS):
        for c, runs in enumerate(row):
            if not runs or (model == "58" and runs == [("Write", R)]):
                continue
            x, y, w, h = well(r, c)
            box = INK["legend"][r][c] if INK else None
            draw_label(runs, LS, LEGEND, stroke=LSTROKE, cx=x + w / 2, base=y - LEGEND_GAP,
                       track=legend_track(runs, box[2]) if box else 0.0)
    end()


# ---------------------------------------------------------------- cue card window
def cue_card(card_png):
    show = card_png is not None
    layer("cue-card", "" if show else ' style="display:none"')
    emit('<g id="card" clip-path="url(#cardClip)">')
    if show:
        b64 = base64.b64encode(Path(card_png).read_bytes()).decode()
        emit(f'<image x="{CARD["x"]}" y="{CARD["y"]}" width="{CARD["w"]}" height="{CARD["h"]}" '
             f'preserveAspectRatio="none" href="data:image/png;base64,{b64}"/>')
    else:
        emit(f'<rect id="card-window" x="{CARD["x"]}" y="{CARD["y"]}" width="{CARD["w"]}" height="{CARD["h"]}" '
             f'fill="none" stroke="#c49223" stroke-width="4" stroke-dasharray="24 16"/>')
    emit("</g>")
    emit(f'<rect id="card-wash" x="{CARD["x"]}" y="{CARD["y"]}" width="{CARD["w"]}" height="{WASH_H}" '
         f'fill="{WASH}" fill-opacity="{WASH_ALPHA}"/>')
    end()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", choices=("59", "58"), default="59", help="58: TI-58 / TI-58C (no Write legend)")
    ap.add_argument("--led", default="", help="sample LED digits, e.g. 3.141592654 (0-9 - E C space, '.' after a digit)")
    ap.add_argument("--card", help="cue card art (2064 x 440 PNG) to fit into the card window, with its wash")
    ap.add_argument("--legend", choices=LEGENDS_INK, default="warm", help="legend ink (colour or gradient)")
    ap.add_argument("--keep-svg", action="store_true", help="keep the SVG next to the PNG")
    ap.add_argument("--no-png", action="store_true", help="write the SVG and layout only (implies --keep-svg)")
    ap.add_argument("-o", "--out", required=True, help="SVG path (PNG and layout JSON next to it)")
    a = ap.parse_args()
    svg = Path(a.out)
    global LEGEND, LEGEND_NAME
    LEGEND_NAME = a.legend
    ink = LEGENDS_INK[a.legend]
    LEGEND = ink[0] if len(ink) == 1 else "url(#legendInk)"
    svgkit.out.clear()
    emit(svgkit.svg_open(W, H))
    defs()
    body()
    display(a.led)
    printed_text(a.model)
    keys()
    cue_card(a.card)
    emit("</svg>")
    text = "\n".join(svgkit.out)
    cw = DISPLAY["w"] / CELLS
    layout = {"size": [W, H], "display_panel": DISPLAY_PANEL, "display": DISPLAY,
              "led": {"cells": CELLS, "cell_w": cw, "cell_h": DISPLAY["h"], "slant": SLANT,
                      "cell_x": {str(i): round(DISPLAY["x"] + (CELLS - 1 - i) * cw, 2) for i in range(CELLS)}},
              "card": CARD, "card_wash": dict(CARD, h=WASH_H, color=WASH, alpha=WASH_ALPHA),
              "keys": svgkit.keymap(text)}
    svgkit.finish(svg, text, layout, a.keep_svg, a.no_png)


if __name__ == "__main__":
    main()
