#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10,<3.15"
# dependencies = ["fonttools", "pyoxipng"]
# ///
"""Generate a layered, semantic SVG of the Sharp PC-1500 / PC-1500A front panel.

Coordinate system: 4000 x 1760 units; the key wells, the dark band, the LCD bezel and the key
shelf were measured on a reference drawing of that size.
Colours, key caps and printed details follow 600 dpi scans of a PC-1500 and a PC-1500A
(not in the repo). The two models share the geometry; their differences are listed in MODELS.
Fonts, text helper and output are in tools/faceplate/svgkit.py.

Layers (bottom to top):
  body        brushed aluminium plate
  print-areas dark display band, key shelf under the LCD
  lighting    plate shading, bevelled edges
  print-lines band rule, shelf dividers and frame, POWER line or box
  print-text  printed labels (shift functions, captions, logo)
  display     LCD bezel, window, LCD background, dots and status segments
  keys        key wells, caps and cap labels

Usage: build_svg_1500.py [--model 1500|1500a] [--text '10 PRINT "HELLO"'] [--cursor N] [--segments DE,G,RUN,BATT]
                         [--keep-svg] [--no-png] [-o out.svg]
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
from svgkit import FONT_COND, FONT_GEO, FONT_LOGO, FONT_MATH, FONT_SANS, emit, end, layer, rich
from lcdfont import FONT

W, H = 4000, 1760

# ---------------------------------------------------------------- colours (from the scan)
ALU = "#bcc1c6"
ALU_EDGE = "#7f848a"
BAND_RULE = ALU            # rule under MEMORY SAFE GUARD: plate colour, unbrushed
SHELF = "#a6a6a5"         # matte key shelf under the LCD
SHELF_LINE = "#d4d7da"
BEZEL = "#c9cdd0"         # brushed silver LCD bezel
RED = "#c75a32"           # shift legends: orange-red (hue ~16 deg on both scans)
INK = "#161616"           # black print
LOGO_SIZE = 126           # Calc-U logo font size; fitted width = LOGO_CHAR_W per character
LOGO_CHAR_W = 728 / 11
LEGEND_GAP = 14           # printed legend: lowest ink -> top of the key well
LCD_BG = ("#a1a897", "#969e8e", "#8b9384")
LCD_INK = "#202818"

# ---------------------------------------------------------------- models
#   band        colour of the display band (and of the frame around the key shelf, if any)
#   band_ink    print on the band (MEMORY SAFE GUARD..., POCKET COMPUTER)
#   shelf_frame band-coloured frame around the function-key shelf
#   power_box   framed POWER / OFF / ON group; otherwise a line under it
#   cl          cap colours of the CL key (face, highlight, skirt, label), default CAPS["red"]
MODELS = {
    "1500": dict(name="Calc-U-1500", band="#7d756a", band_ink="#0e0d0c", shelf_frame=False, power_box=False),
    "1500a": dict(name="Calc-U-1500A", band="#2b201d", band_ink="#e9e7e3", shelf_frame=True, power_box=True,
                  cl=("#912528", "#a4373a", "#651517", "#ffffff")),  # darker red cap (scan 144/38/41)
}
M = MODELS["1500"]  # selected in main()

CAPS = {  # face, face-top highlight, skirt, label
    "dark":   ("#24160f", "#3a2a22", "#3b2a24", "#f4f2ee"),
    "grey":   ("#b3b5b7", "#c6c8ca", "#808285", "#121212"),
    "orange": ("#d8812d", "#e69a4c", "#9e5a1c", "#121212"),
    "red":    ("#b3262b", "#c53c3e", "#7e171b", "#ffffff"),
}

# ---------------------------------------------------------------- geometry (PC-1500.png)
PLATE = dict(x=4, y=4, w=3992, h=1752, r=50)
BAND_R = dict(x=52, y=66, w=3895, h=553, r=36)
RULE_Y = 163
BEZEL_R = dict(x=431, y=218, w=2465, h=333, r=22)
WINDOW = dict(x=489, y=264, w=2348, h=239, r=16)
LCD = dict(x=495, y=270, w=2336, h=227)       # visible LCD glass
SHELF_R = dict(x=592, y=540, w=2051, h=297)
SHELF_DIV = [957.5, 1286.5, 1616.5, 1946.5, 2276.5]

PITCH = 247.6
KW, KH = 177, 125            # key wells, main rows
ROW_Y = [919, 1115, 1311, 1507]
NUM_X = [2659, 2906, 3154, 3402, 3650]
TOP_Y, TOP_H = 694, 111      # DEF and the six shelf keys
TOP2_Y, TOP2_H = 702, 114    # SHIFT, OFF, ON


def col(x0, i):
    return round(x0 + i * PITCH)


# LCD dot matrix: same geometry as Calc-U-1600's LcdWidget (panel 508,280,2309,208)
PANEL = dict(x=508, y=280, w=2309, h=208)
COLS, ROWS = 156, 7
_IX, _IW = PANEL["x"] + 4, PANEL["w"] - 8
_EFF = PANEL["h"] * 24.52 / 31.1
_BAR = _EFF * (18 / 42) / (1 + 18 / 42)
DOT_PX = _IW / COLS
DOT_PY = (_EFF - _BAR) / ROWS
DOT = min(DOT_PX, DOT_PY) * 0.9
MATRIX_Y0 = PANEL["y"] + _BAR + _EFF * 0.2
SEG_FONT = _EFF * 0.277
SEG_Y = PANEL["y"] + _EFF * 0.01
# status segments: name -> (x fraction of the interior, caption)
SEGS = {"BUSY": (0.0053, "BUSY"), "SHIFT": (0.0998, "SHIFT"), "KANA": (0.1943, "JPN"), "SMALL": (0.2572, "SMALL"),
        "DE": (0.3579, "DE"), "G": (0.3579, "G"), "RAD": (0.3579, "RAD"), "RUN": (0.5090, "RUN"),
        "PRO": (0.6034, "PRO"), "RESERVE": (0.6664, "RESERVE"), "DEF": (0.7922, "DEF"), "I": (0.8740, "I"),
        "II": (0.8930, "II"), "III": (0.9245, "III")}
SEG_DX = {"G": 1.12, "RAD": 1.72}   # DE / G / RAD read as one word (DEG, GRAD, RAD): offsets in font sizes
SEG_BOX = {}


# ---------------------------------------------------------------- defs
def defs():
    emit("<defs>")
    emit(f"<style>{B.font_faces()}</style>")
    emit(f"""
<filter id="brushed" x="0" y="0" width="1" height="1" filterUnits="objectBoundingBox" color-interpolation-filters="sRGB">
  <feTurbulence type="fractalNoise" baseFrequency="0.0012 0.30" numOctaves="3" seed="7" result="n1"/>
  <feTurbulence type="fractalNoise" baseFrequency="0.013 0.65" numOctaves="2" seed="3" result="n2"/>
  <feComposite in="n1" in2="n2" operator="arithmetic" k2="0.6" k3="0.4" result="n"/>
  <feColorMatrix in="n" type="matrix" values="0.33 0.33 0.33 0 0  0.33 0.33 0.33 0 0  0.33 0.33 0.33 0 0  0 0 0 0 1" result="g"/>
  <feComposite in="SourceGraphic" in2="g" operator="arithmetic" k2="1" k3="0.42" k4="-0.21" result="mix"/>
  <feComposite in="mix" in2="SourceGraphic" operator="in"/>
</filter>
<filter id="brushedCap" x="0" y="0" width="1" height="1" filterUnits="objectBoundingBox" color-interpolation-filters="sRGB">
  <feTurbulence type="fractalNoise" baseFrequency="0.004 0.4" numOctaves="2" seed="11" result="n"/>
  <feColorMatrix in="n" type="matrix" values="0.33 0.33 0.33 0 0  0.33 0.33 0.33 0 0  0.33 0.33 0.33 0 0  0 0 0 0 1" result="g"/>
  <feComposite in="SourceGraphic" in2="g" operator="arithmetic" k2="1" k3="0.40" k4="-0.20" result="mix"/>
  <feComposite in="mix" in2="SourceGraphic" operator="in"/>
</filter>
<filter id="soft" x="-20%" y="-20%" width="140%" height="160%"><feGaussianBlur stdDeviation="6"/></filter>
<filter id="soft2" x="-20%" y="-20%" width="140%" height="160%"><feGaussianBlur stdDeviation="2.4"/></filter>
<filter id="lcdshadow" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur stdDeviation="2"/></filter>
<linearGradient id="aluLight" x1="0" y1="0" x2="0.35" y2="1">
  <stop offset="0" stop-color="#ffffff" stop-opacity="0.18"/>
  <stop offset="0.5" stop-color="#ffffff" stop-opacity="0"/>
  <stop offset="1" stop-color="#000000" stop-opacity="0.10"/>
</linearGradient>
<linearGradient id="lip" x1="0" y1="0" x2="0" y2="1">
  <stop offset="0" stop-color="#ffffff" stop-opacity="0"/>
  <stop offset="0.25" stop-color="#ffffff" stop-opacity="0.45"/>
  <stop offset="0.8" stop-color="#ffffff" stop-opacity="0.25"/>
  <stop offset="1" stop-color="#000000" stop-opacity="0.25"/>
</linearGradient>
<linearGradient id="bandShade" x1="0" y1="0" x2="0" y2="1">
  <stop offset="0" stop-color="#000000" stop-opacity="0.25"/>
  <stop offset="0.12" stop-color="#000000" stop-opacity="0"/>
  <stop offset="0.85" stop-color="#ffffff" stop-opacity="0"/>
  <stop offset="1" stop-color="#ffffff" stop-opacity="0.10"/>
</linearGradient>
<linearGradient id="bezelLight" x1="0" y1="0" x2="0" y2="1">
  <stop offset="0" stop-color="#ffffff" stop-opacity="0.35"/>
  <stop offset="0.5" stop-color="#ffffff" stop-opacity="0"/>
  <stop offset="1" stop-color="#000000" stop-opacity="0.12"/>
</linearGradient>
<linearGradient id="lcdGrad" x1="0" y1="0" x2="1" y2="0.3">
  <stop offset="0" stop-color="{LCD_BG[0]}"/>
  <stop offset="0.55" stop-color="{LCD_BG[1]}"/>
  <stop offset="1" stop-color="{LCD_BG[2]}"/>
</linearGradient>
<linearGradient id="capShade" x1="0" y1="0" x2="1" y2="0">
  <stop offset="0" stop-color="#ffffff" stop-opacity="0.10"/>
  <stop offset="0.75" stop-color="#000000" stop-opacity="0"/>
  <stop offset="1" stop-color="#000000" stop-opacity="0.18"/>
</linearGradient>
<linearGradient id="darkGloss" x1="0" y1="0" x2="0" y2="1">
  <stop offset="0" stop-color="#ffffff" stop-opacity="0.14"/>
  <stop offset="0.45" stop-color="#ffffff" stop-opacity="0.02"/>
  <stop offset="1" stop-color="#000000" stop-opacity="0.15"/>
</linearGradient>
<filter id="capFinish" x="0" y="0" width="1" height="1" filterUnits="objectBoundingBox" color-interpolation-filters="sRGB">
  <feTurbulence type="fractalNoise" baseFrequency="0.003 0.25" numOctaves="1" seed="5" result="n"/>
  <feColorMatrix in="n" type="matrix" values="0.33 0.33 0.33 0 0  0.33 0.33 0.33 0 0  0.33 0.33 0.33 0 0  0 0 0 0 1" result="g"/>
  <feComposite in="SourceGraphic" in2="g" operator="arithmetic" k2="1" k3="0.10" k4="-0.05" result="mix"/>
  <feComposite in="mix" in2="SourceGraphic" operator="in"/>
</filter>
<!-- silver logo like the SHARP badge: bright metal with fine vertical hairlines, a sheen and a dark rim -->
<pattern id="logoLines" patternUnits="userSpaceOnUse" width="7" height="20">
  <rect width="7" height="20" fill="#eceef0"/>
  <rect x="4.6" width="2.4" height="20" fill="#7b8085"/>
</pattern>
<linearGradient id="logoSheen" x1="0" y1="0" x2="0" y2="1">
  <stop offset="0" stop-color="#ffffff" stop-opacity="0.55"/>
  <stop offset="0.45" stop-color="#ffffff" stop-opacity="0.05"/>
  <stop offset="0.55" stop-color="#000000" stop-opacity="0.10"/>
  <stop offset="1" stop-color="#ffffff" stop-opacity="0.30"/>
</linearGradient>
<clipPath id="plateClip"><rect x="{PLATE['x']}" y="{PLATE['y']}" width="{PLATE['w']}" height="{PLATE['h']}" rx="{PLATE['r']}"/></clipPath>
""")
    emit("</defs>")


def rrect(r, **attrs):
    a = " ".join(f'{k.replace("_", "-")}="{v}"' for k, v in attrs.items())
    return f'<rect x="{r["x"]}" y="{r["y"]}" width="{r["w"]}" height="{r["h"]}" rx="{r.get("r", 0)}" {a}/>'


# ---------------------------------------------------------------- body & prints
def body_and_prints():
    # plate + printed areas inside one filtered group -> brushing goes through the ink
    emit('<g id="brushed-plate" filter="url(#brushed)">')
    layer("body")
    emit(rrect(PLATE, id="plate", fill=ALU))
    end()
    layer("print-areas")
    emit(rrect(BAND_R, id="band", fill=M["band"]))
    emit(rrect(SHELF_R, id="key-shelf", fill=SHELF))
    end()
    emit("</g>")

    layer("lighting")
    emit(rrect(PLATE, fill="url(#aluLight)"))
    emit(rrect(BAND_R, fill="url(#bandShade)"))
    # recessed band: dark inner edge at the top/left, bright alu lip around it
    b = BAND_R
    emit(f'<rect x="{b["x"] - 3}" y="{b["y"] - 3}" width="{b["w"] + 6}" height="{b["h"] + 6}" rx="{b["r"] + 3}" '
         f'fill="none" stroke="#f2f4f6" stroke-opacity="0.8" stroke-width="4"/>')
    emit(rrect(BAND_R, fill="none", stroke="#141010", stroke_opacity="0.75", stroke_width="3"))
    # shelf edges (its top lies under the bezel)
    s = SHELF_R
    emit(f'<path d="M{s["x"]},{s["y"]} V{s["y"] + s["h"]} H{s["x"] + s["w"]} V{s["y"]}" fill="none" '
         f'stroke="#55585b" stroke-opacity="0.55" stroke-width="3"/>')
    # bottom lip of the case
    emit(f'<g clip-path="url(#plateClip)"><rect x="0" y="1686" width="{W}" height="74" fill="url(#lip)"/></g>')
    # bevelled plate edge
    emit(f'<rect x="{PLATE["x"] + 2}" y="{PLATE["y"] + 2}" width="{PLATE["w"] - 4}" height="{PLATE["h"] - 4}" '
         f'rx="{PLATE["r"] - 2}" fill="none" stroke="#f1f3f5" stroke-opacity="0.7" stroke-width="3"/>')
    emit(rrect(PLATE, fill="none", stroke=ALU_EDGE, stroke_width="3"))
    end()

    layer("print-lines")
    emit(f'<rect id="band-rule" x="{b["x"] + 4}" y="{RULE_Y}" width="{b["w"] - 8}" height="7" fill="{BAND_RULE}"/>')
    divs = " ".join(f"M{x},{s['y'] + 14} V{s['y'] + s['h'] - 2}" for x in SHELF_DIV)
    emit(f'<path id="shelf-dividers" d="{divs}" stroke="{SHELF_LINE}" stroke-width="5"/>')
    if M["shelf_frame"]:  # left, bottom and right side; the top meets the band
        emit(f'<path id="shelf-frame" d="M{s["x"]},{BAND_R["y"] + BAND_R["h"] - 2} V{s["y"] + s["h"]} '
             f'H{s["x"] + s["w"]} V{BAND_R["y"] + BAND_R["h"] - 2}" fill="none" stroke="{M["band"]}" stroke-width="8"/>')
    if M["power_box"]:
        emit(f'<rect id="power-box" x="3158" y="646" width="785" height="196" rx="6" fill="none" '
             f'stroke="{INK}" stroke-width="4"/>')
    else:
        emit(f'<path id="power-line" d="M3158,833 H3826" stroke="{INK}" stroke-width="4"/>')
    end()


# ---------------------------------------------------------------- keys
def icon(kind, cx, cy, s, color):
    k = s / 20.0
    if kind == "updown":  # RSV rocker: up and down triangle
        emit(f'<path d="M{cx - 9*k:.1f},{cy - 2*k:.1f} L{cx:.1f},{cy - 13*k:.1f} L{cx + 9*k:.1f},{cy - 2*k:.1f} Z '
             f'M{cx - 9*k:.1f},{cy + 2*k:.1f} L{cx:.1f},{cy + 13*k:.1f} L{cx + 9*k:.1f},{cy + 2*k:.1f} Z" fill="{color}"/>')
    elif kind == "aster":  # six-armed bold asterisk
        d = " ".join(f"M{cx - 11*k*c:.1f},{cy - 11*k*sn:.1f} L{cx + 11*k*c:.1f},{cy + 11*k*sn:.1f}"
                     for c, sn in ((0, 1), (0.866, 0.5), (0.866, -0.5)))
        emit(f'<path d="{d}" stroke="{color}" stroke-width="{3.8*k:.1f}" stroke-linecap="butt"/>')
    elif kind == "bar":  # minus
        emit(f'<rect x="{cx - 12*k:.1f}" y="{cy - 1.7*k:.1f}" width="{24*k:.1f}" height="{3.4*k:.1f}" fill="{color}"/>')
    elif kind == "plus":
        emit(f'<path d="M{cx - 12*k:.1f},{cy:.1f} H{cx + 12*k:.1f} M{cx:.1f},{cy - 12*k:.1f} V{cy + 12*k:.1f}" '
             f'stroke="{color}" stroke-width="{3.4*k:.1f}"/>')
    elif kind == "equal":
        emit(f'<path d="M{cx - 12*k:.1f},{cy - 5*k:.1f} H{cx + 12*k:.1f} M{cx - 12*k:.1f},{cy + 5*k:.1f} H{cx + 12*k:.1f}" '
             f'stroke="{color}" stroke-width="{3.2*k:.1f}"/>')
    else:
        B.icon(kind, cx, cy, s, color)


def key(ident, x, y, w, h, scheme, label=None, size=90, family=FONT_SANS, weight=400, fit=None):
    face, face_top, skirt, lcol = M.get("cl", CAPS["red"]) if scheme == "red" else CAPS[scheme]
    # cap inside the well: fixed margins like the drawing (well 177 x 125, cap 145 x 95)
    cx, cy, cw, ch = x + 16, y + 14, w - 32, h - 28
    emit(f'<g id="key-{ident}" class="key" data-x="{x}" data-y="{y}" data-w="{w}" data-h="{h}">')
    emit(f'<rect x="{x + 6}" y="{y + 9}" width="{w}" height="{h}" rx="18" fill="#000" fill-opacity="0.30" filter="url(#soft)"/>')
    emit(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="16" fill="#0b0908"/>')
    emit(f'<rect x="{x + 1.5}" y="{y + 1.5}" width="{w - 3}" height="{h - 3}" rx="15" fill="none" '
         f'stroke="#ffffff" stroke-opacity="0.10" stroke-width="2"/>')
    emit(f'<rect x="{cx + 2}" y="{cy + 4}" width="{cw + 2}" height="{ch + 4}" rx="6" fill="#000" fill-opacity="0.6" filter="url(#soft2)"/>')
    emit(f'<rect x="{cx}" y="{cy}" width="{cw}" height="{ch}" rx="6" fill="{skirt}"/>')
    fh = ch - 6
    brushed = ' filter="url(#capFinish)"' if scheme == "grey" else ""  # satin, much finer than the plate
    emit(f'<rect x="{cx}" y="{cy}" width="{cw}" height="{fh}" rx="6" fill="{face}"{brushed}/>')
    emit(f'<rect x="{cx + 1.5}" y="{cy + 1.2}" width="{cw - 3}" height="7" rx="4" fill="{face_top}"/>')
    emit(f'<rect x="{cx}" y="{cy}" width="{cw}" height="{fh}" rx="6" fill="url(#{"darkGloss" if scheme == "dark" else "capShade"})"/>')
    emit(f'<line x1="{cx + 5}" y1="{cy + 1.2}" x2="{cx + cw - 5}" y2="{cy + 1.2}" stroke="#fff" stroke-opacity="0.35" stroke-width="1.8"/>')
    lx, ly = cx + cw / 2, cy + fh / 2
    if isinstance(label, tuple) and label[0] == "icon":
        icon(label[1], lx, ly, label[2] if len(label) > 2 else 40, lcol)
    elif label is not None:
        rich(lx, ly + 4.5, label, size, color=lcol, family=family, weight=weight, fit=fit, vcap=True)
    emit("</g>")


def keys():
    layer("keys")
    # key legends: Helvetica regular (TeX Gyre Heros), sizes and widths measured on PC-1500.png
    COND = dict(family=FONT_COND, weight=400)
    SANS = dict(family=FONT_SANS, weight=400)
    # top row
    key("def", 184, TOP_Y, 178, TOP_H, "grey", "DEF", size=65, fit=115, **SANS)
    for i in range(6):
        key(f"f{i + 1}", round(704 + i * 330), TOP_Y, 177, TOP_H, "grey")
    key("shift", 2906, TOP2_Y, 178, TOP2_H, "orange", "SHIFT", size=60, fit=131, **COND)
    key("off", 3402, TOP2_Y, 177, TOP2_H, "grey", "OFF", size=68, fit=117, **SANS)
    key("on", 3650, TOP2_Y, 178, TOP2_H, "grey", "ON", size=67, fit=84, **SANS)
    # letters
    for r, (x0, letters) in enumerate(((184, "QWERTYUIOP"), (246, "ASDFGHJKL"), (367, "ZXCVBNM()"))):
        for i, ch in enumerate(letters):
            ident = {"(": "lpar", ")": "rpar"}.get(ch, ch.lower())
            key(ident, col(x0, i), ROW_Y[r], KW, KH, "dark", ch, size=86)
    # bottom row
    y = ROW_Y[3]
    key("sml", col(367, 0), y, KW, KH, "grey", "SML", size=68, fit=122, **SANS)
    key("rsv", col(367, 1), y, KW, KH, "grey", ("icon", "updown", 56))
    key("rcl", col(367, 2), y, KW, KH, "grey", "RCL", size=69, fit=115, **SANS)
    key("space", col(367, 3), y, 424, KH, "dark", "SPACE", size=68, fit=196, **COND)
    key("down", col(367, 5), y, KW, KH, "grey", ("icon", "down", 74))
    key("up", col(367, 6), y, KW, KH, "grey", ("icon", "up", 74))
    key("enter", col(367, 7), y, 422, KH, "grey", "ENTER", size=68, fit=198, **COND)
    # numeric block
    num = [["7", "8", "9", "slash", "cl"], ["4", "5", "6", "aster", "mode"],
           ["1", "2", "3", "minus", "left"], ["0", "dot", "equal", "plus", "right"]]
    for r, row in enumerate(num):
        for i, n in enumerate(row):
            x, y = NUM_X[i], ROW_Y[r] + 1
            if n.isdigit():
                key(n, x, y, 176, KH, "grey", n, size=89)
            elif n == "slash":
                key(n, x, y, 177, KH, "grey", "/", size=89, fit=42, **SANS)
            elif n == "cl":
                key(n, x, y, 178, KH, "red", "CL", size=82, fit=102, **SANS)
            elif n == "mode":
                key(n, x, y, 178, KH, "grey", "MODE", size=64, fit=131, **COND)
            elif n == "dot":
                key(n, x, y, 177, KH, "grey", ("icon", "dot", 46))
            elif n in ("left", "right"):
                key(n, x, y, 177, KH, "grey", ("icon", n, 62))
            else:
                key(n, x, y, 177, KH, "grey", ("icon", {"aster": "aster", "minus": "bar", "equal": "equal",
                                                       "plus": "plus"}[n], 46))
    end()


# ---------------------------------------------------------------- printed text
def printed_text():
    layer("print-text")
    # band
    rich(874, 120, "MEMORY SAFE GUARD / AUTO POWER OFF", 54, color=M["band_ink"], weight=400, fit=884, vcap=True,
         family=FONT_COND)
    # silver logo, right-aligned with POCKET COMPUTER: hairline metal, then sheen, then a dark rim
    lw = LOGO_CHAR_W * len(M["name"])
    logo = dict(family=FONT_LOGO, weight=700, fit=lw, vcap=True)
    rich(3760 - lw / 2, 322, M["name"], LOGO_SIZE, color="url(#logoLines)", **logo)
    rich(3760 - lw / 2, 322, M["name"], LOGO_SIZE, color="url(#logoSheen)", **logo)
    rich(3760 - lw / 2, 322, M["name"], LOGO_SIZE, color="none",
         style="stroke:#2b2622;stroke-width:2.2;stroke-linejoin:round", **logo)
    rich(3496, 483, "POCKET COMPUTER", 52, color=M["band_ink"], weight=400, fit=528, vcap=True)

    # legends above the keys: the lowest ink of every legend sits LEGEND_GAP above the well
    def above(x, w, y, segs, size=56, color=RED, **kw):
        base = y - LEGEND_GAP + ink_bottom(segs, size)
        rich(x + w / 2, base - 0.35 * size, segs, size, color=color, weight=400, vcap=True, **kw)
    for i, t in enumerate(['!', '"', '#', '$', '%', '&']):
        x = 704 + i * 330 + 177 / 2
        if t == '"':  # two tall ticks, as printed (taller than the font's quote)
            y1 = TOP_Y - LEGEND_GAP
            emit(f'<path d="M{x - 8},{y1 - 36} V{y1} M{x + 8},{y1 - 36} V{y1}" stroke="{RED}" stroke-width="6"/>')
        else:
            above(704 + i * 330, 177, TOP_Y, t, size=52 if t == "$" else 60)
    above(3650, 178, TOP2_Y, "BREAK", size=46, color=INK)
    if M["power_box"]:  # inside the box, level with OFF / ON
        rich(3292, TOP2_Y + TOP2_H / 2, "POWER", 44, color=INK, weight=400, vcap=True)
    else:  # on the line
        rich(3160, 796, "POWER", 44, color=INK, weight=400, anchor="start", vcap=False)
    for (i, r), t in {(3, 0): "?", (4, 0): "CA", (3, 1): ":", (3, 2): ",", (4, 2): "DEL", (2, 3): "@", (3, 3): ";",
                      (4, 3): "INS"}.items():
        above(NUM_X[i], 177, ROW_Y[r] + 1, t, size=54 if len(t) > 1 or t == "@" else 60)
    above(col(367, 7), KW, ROW_Y[2], "<", fit=50)  # printed wider than the font's angle brackets
    above(col(367, 8), KW, ROW_Y[2], ">", fit=50)
    above(col(367, 3), 424, ROW_Y[3], "^", size=60)
    above(col(367, 5), KW, ROW_Y[3], [("π", "i")], size=66)
    # root sign: B.icon draws its lowest point 10 units (size/20) below its centre
    x, s = col(367, 6) + KW / 2, 34
    icon("sqrt", x, ROW_Y[3] - LEGEND_GAP - 10 * s / 20 - 1.8 * s / 20, s, RED)
    end()


_FONTS = {}


def ink_bottom(segs, size):
    """Lowest ink of a legend relative to its baseline, in units (positive = above the baseline)."""
    from fontTools.pens.boundsPen import BoundsPen
    from fontTools.ttLib import TTFont
    if isinstance(segs, str):
        segs = [(segs, "")]
    lo = None
    for text, flags in segs:
        fn = "texgyretermes-italic.otf" if "i" in flags else "texgyreheros-regular.otf"
        if fn not in _FONTS:
            _FONTS[fn] = TTFont(B.FONTS / fn)
        font = _FONTS[fn]
        gs, cmap = font.getGlyphSet(), font.getBestCmap()
        upm = font["head"].unitsPerEm
        for ch in text:
            if ch == " ":
                continue
            pen = BoundsPen(gs)
            gs[cmap[ord(ch)]].draw(pen)
            if pen.bounds:
                y = pen.bounds[1] * size / upm
                lo = y if lo is None else min(lo, y)
    return lo or 0.0


# ---------------------------------------------------------------- display
def display(text, cursor, segments):
    layer("display")
    b = BEZEL_R
    emit(f'<rect x="{b["x"] + 4}" y="{b["y"] + 8}" width="{b["w"]}" height="{b["h"]}" rx="{b["r"]}" fill="#000" '
         f'fill-opacity="0.45" filter="url(#soft)"/>')
    emit(rrect(BEZEL_R, id="bezel", fill=BEZEL, filter="url(#brushedCap)"))
    emit(rrect(BEZEL_R, fill="url(#bezelLight)"))
    emit(rrect(BEZEL_R, fill="none", stroke="#2a2a2a", stroke_width="3"))
    emit(f'<rect x="{b["x"] + 3}" y="{b["y"] + 3}" width="{b["w"] - 6}" height="{b["h"] - 6}" rx="{b["r"] - 3}" '
         f'fill="none" stroke="#ffffff" stroke-opacity="0.6" stroke-width="2.5"/>')
    emit(rrect(WINDOW, fill="#151515"))
    emit(rrect(LCD, id="lcd-bg", fill="url(#lcdGrad)"))
    # inner shadow of the window frame on the glass
    emit(f'<path d="M{LCD["x"]},{LCD["y"]} H{LCD["x"] + LCD["w"]} V{LCD["y"] + 8} H{LCD["x"] + 8} V{LCD["y"] + LCD["h"]} '
         f'H{LCD["x"]} Z" fill="#000" fill-opacity="0.25"/>')
    dots(text, cursor, segments)
    emit(f'<path d="M{LCD["x"]},{LCD["y"]} H{LCD["x"] + LCD["w"]} V{LCD["y"] + 30} L{LCD["x"]},{LCD["y"] + 48} Z" '
         f'fill="#ffffff" fill-opacity="0.05"/>')
    end()


def dots(text, cursor, segments):
    emit('<g id="lcd-dots">')
    on, ghost = [], []
    for c in range(COLS // 6):
        ch = text[c] if c < len(text) else " "
        glyph = FONT.get(ch, FONT["?"])
        if c == cursor:
            glyph = glyph[:6] + ["11111"]
        for ry, row in enumerate(glyph):
            for rx in range(6):
                bit = row[rx] if rx < 5 else "0"
                p = (_IX + (c * 6 + rx + 0.5) * DOT_PX - DOT / 2, MATRIX_Y0 + (ry + 0.5) * DOT_PY - DOT / 2)
                (on if bit == "1" else ghost).append(p)

    def rects(pts, dx=0, dy=0):
        return "".join(f"M{x + dx:.2f},{y + dy:.2f}h{DOT:.2f}v{DOT:.2f}h-{DOT:.2f}z" for x, y in pts)
    emit(f'<path id="lcd-ghost" d="{rects(ghost)}" fill="#000" fill-opacity="0.018"/>')
    emit(f'<path id="lcd-dot-shadow" d="{rects(on, 3, 3.5)}" fill="#3d4530" fill-opacity="0.30" filter="url(#lcdshadow)"/>')
    emit(f'<path id="lcd-on" d="{rects(on)}" fill="{LCD_INK}" fill-opacity="0.93"/>')
    emit("</g>")

    emit('<g id="lcd-segments">')
    for name, (fx, txt) in SEGS.items():
        x = _IX + fx * _IW + SEG_DX.get(name, 0) * SEG_FONT
        op = 0.92 if name in segments else 0.0
        w = len(txt) * SEG_FONT * 0.52
        SEG_BOX[name] = dict(x=round(x, 1), y=round(SEG_Y, 1), w=round(w, 1), h=round(SEG_FONT, 1))
        emit(f'<text id="seg-{name}" class="segment" opacity="{op}" x="{x:.1f}" y="{SEG_Y + SEG_FONT * 0.82:.1f}" '
             f'font-family="{FONT_COND}" font-size="{SEG_FONT:.1f}" font-weight="700" fill="{LCD_INK}" '
             f'textLength="{w:.1f}" lengthAdjust="spacingAndGlyphs">{txt}</text>')
    # battery dot: always on
    d = SEG_FONT * 0.4
    x = _IX + 0.9874 * _IW
    SEG_BOX["BATT"] = dict(x=round(x - d, 1), y=round(SEG_Y + d / 2, 1), w=round(d, 1), h=round(d, 1))
    emit(f'<circle id="seg-BATT" class="segment" cx="{x - d / 2:.1f}" cy="{SEG_Y + d:.1f}" r="{d / 2:.1f}" '
         f'fill="{LCD_INK}" opacity="{0.92 if "BATT" in segments else 0.0}"/>')
    emit("</g>")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--text", default="", help='LCD text, e.g. \'10 PRINT "HELLO, WORLD!"\' (default: LCD off)')
    ap.add_argument("--cursor", type=int, default=-1, help="cursor column (default: none)")
    ap.add_argument("--segments", default="", help="lit status segments, e.g. DE,G,RUN,BATT (BATT = battery dot)")
    ap.add_argument("--model", choices=MODELS, default="1500")
    ap.add_argument("--keep-svg", action="store_true", help="keep the SVG next to the PNG")
    ap.add_argument("--no-png", action="store_true", help="write the SVG and layout only")
    ap.add_argument("-o", "--out", required=True, help="SVG path (PNG and layout JSON next to it)")
    ap.add_argument("--band", help="colour of the display band, e.g. #6c645a")
    a = ap.parse_args()
    global M
    M = dict(MODELS[a.model], **({"band": a.band} if a.band else {}))
    out = a.out
    B.out.clear()
    emit(f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" '
         f'width="{W}" height="{H}" viewBox="0 0 {W} {H}">')
    defs()
    body_and_prints()
    printed_text()
    display(a.text.upper(), a.cursor, set(a.segments.split(",")) - {""})
    keys()
    emit("</svg>")
    svg = "\n".join(B.out)
    keymap = {m[0]: dict(x=float(m[1]), y=float(m[2]), w=float(m[3]), h=float(m[4]))
              for m in re.findall(r'id="key-([^"]+)" class="key" data-x="([^"]+)" data-y="([^"]+)" '
                                  r'data-w="([^"]+)" data-h="([^"]+)"', svg)}
    lcd = dict(PANEL, cols=COLS, rows=ROWS, matrix_x0=round(_IX, 2), matrix_y0=round(MATRIX_Y0, 2),
               dot_pitch_x=round(DOT_PX, 4), dot_pitch_y=round(DOT_PY, 4), dot=round(DOT, 4))
    B.finish(out, svg, {"size": [W, H], "keys": keymap, "lcd": lcd, "segments": SEG_BOX},
             a.keep_svg, a.no_png)


if __name__ == "__main__":
    main()
