#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10,<3.15"
# dependencies = ["fonttools", "pyoxipng"]
# ///
"""Generate the layered, semantic SVG of the PC-1401 / PC-1402 / PC-1403H / PC-1421 faceplate.

Coordinate system: 2576 x 1150 units; the geometry was measured on a photo of a PC-1403H scaled
to 2576 px width. The four models share the panel; their differences are listed in MODELS.
Fonts, font metrics, the output buffer, rich text and the rendering are in tools/faceplate/svgkit.py.

Layers (bottom to top); body and print-areas lie in the group brushed-plate, whose filter
brushes the plate and the print together:
  case              dark lower case under the plate
  body              aluminium plate
  print-areas       tinted areas: power panel, BASIC panel, grey bands (PC-1421: cream panels)
  lighting          plate shading, bevelled edge
  print-areas-solid unbrushed bars: MATRIX bar, STAT bar (PC-1421: the dark COMP box)
  print-lines       printed frame lines
  print-text        printed labels (shift functions, captions)
  display           bezel with logo, model and memory badge, LCD background, glass captions,
                    dots and status segments (off unless lit)
  power-switch      the POWER slide switch
  keys              key wells, caps and cap labels; data-x/y/w/h = the key well

Usage: build_svg.py [--model 1401|1402|1403h|1421] [--text '10PRINT"Hello, World!"'] [--cursor N]
                    [--segments SML,DEG,PRO] [--keep-svg] [--no-png] -o out.svg
Writes the SVG to -o, the PNG next to it (rendered with headless Chrome, losslessly optimised
with oxipng; the SVG is deleted unless --keep-svg) and <name>-layout.json (key wells, LCD dot geometry, segment boxes).
Without --text, --cursor and --segments the LCD is off: the faceplate; with them, a preview.
"""
import argparse
import sys
from pathlib import Path
from xml.sax.saxutils import escape

sys.dont_write_bytecode = True   # keep the source folders free of __pycache__
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "tools" / "faceplate"))
import svgkit  # noqa: E402  (fonts, output buffer, rich text, rendering)
from svgkit import (BLACK_PRINT, FONT_COND, FONT_GEO, FONT_LOGO, FONT_SANS, RED_PRINT,  # noqa: E402
                    emit, end, icon, layer, rich)
from lcdfont import FONT  # noqa: E402

W, H = 2576, 1150

# ---------------------------------------------------------------- colours
ALU = "#b9bec4"
ALU_EDGE = "#8d9298"
BAND = "#a7a49d"          # grey printed bands (right section)
PANEL = "#bdb6a8"         # warm grey printed panels (left)
BAR_RED = "#a94a3d"
FIN_PANEL = "#cdc8a3"     # PC-1421: cream printed areas (power panel, financial keys)
FIN_BOX = "#2b221b"       # PC-1421: dark box behind COMP
FIN_ORANGE = "#e08a45"    # PC-1421: RCL on the dark box

CAPS = {
    #          face      face-top  skirt     label
    "grey":   ("#c3c0ba", "#d2cfc9", "#9d9a94", "#121212"),
    "brown":  ("#3d3123", "#54452f", "#2a2219", "#f4f3ef"),
    "green":  ("#2f5a4a", "#3d6c5b", "#21423a", "#e2ece6"),
    "yellow": ("#f2ab14", "#f7bd3a", "#c98a08", "#121212"),
    "red":    ("#ee5530", "#f46a45", "#c03e20", "#ffffff"),
    # PC-1401/1402: brushed-metal caps in the plate colour (ALU), deep red C·CE
    "metal":  ("#b9bdc2", "#cbcfd3", "#868a8e", "#161616"),
    "darkred": ("#b5252a", "#c63a3c", "#83181c", "#ffffff"),
}
BLUE_LABEL = "#4db6ee"

# ---------------------------------------------------------------- models
# The PC-1403H is the base; the PC-1401/1402 only list what differs from it.
#   badge       memory badge left of the KB frame: (big digits, small decimals, width) or None
#   matrix      MATRIX frame/bar/caption on the keyboard, MATRIX caption + segment on the LCD
#   sml         SML key and SML status segment
#   lowercase   LCD can show lower case (otherwise the text is upper-cased)
#   glass       printed captions right of the LCD: name -> (caption x, segment bar x)
#   panels      warm grey tinted power panel and BASIC keyword panel (A-, and Z-SPC rows)
#   divider     interrupted vertical line between the left and right keyboard halves
#   grey_keys   cap scheme of all grey keys; cce = cap scheme of C·CE
#   fse         legend of the F<->E key
#   stat_bar, logo_rule, bezel (3 stops), lcd (3 stops)   colours
#   fin         PC-1421 financial keyboard: cream panels, financial keys + labels, other glass captions
#   subtitle    printed text right of the logo, or None
MODELS = {"1403h": dict(
    name="Calc-U-1403H", badge=("32", "", 64), matrix=True, sml=True, lowercase=True,
    glass={"MATRIX": (1161, 1188), "STAT": (1250, 1250), "PRINT": (1327, 1312)}, segments="SML,DEG,PRO",
    panels=True, divider=False, grey_keys="grey", cce="red", fse="FSE",
    stat_bar=BAR_RED, logo_rule="#c46c34", bezel=("#6f6557", "#5a5044", "#3c3328"),
    lcd=("#c6cc8c", "#b9c07c", "#a9b16f"), fin=False, subtitle=None)}
_PC140X = dict(
    matrix=False, sml=False, lowercase=False,
    glass={"STAT": (1180, 1188), "PRINT": (1254, 1250)}, segments="DEG,PRO",
    panels=False, divider=True, grey_keys="metal", cce="darkred", fse=[("F", ""), ("↔", ""), ("E", "")],
    bezel=("#4a3727", "#33251a", "#1e150e"), lcd=("#bfc39b", "#b2b78e", "#a5a983"))
MODELS["1402"] = dict(MODELS["1403h"], **_PC140X, name="Calc-U-1402", badge=("10", ".2", 84))
MODELS["1401"] = dict(MODELS["1403h"], **_PC140X, name="Calc-U-1401", badge=None,
                      stat_bar="#a8592b", logo_rule="#cfc8bc")
MODELS["1421"] = dict(MODELS["1401"], name="Calc-U-1421", fin=True, subtitle="Business/Financial Computer", grey_keys="grey",
                      glass={}, segments="PRO")
# PC-1421 glass captions (measured on the photo): text -> (caption x, segment bar x or None, inverted box width)
FIN_GLASS = {"PRINT": (587, 587, 0), "FIN": (665, 665, 0), "STAT": (719, 719, 0), "CAL": (800, None, 70),
             "RUN": (1157, 1157, 0), "PRO": (1220, 1220, 0), "BASIC": (1310, None, 88)}
M = MODELS["1403h"]  # selected in main()

# ---------------------------------------------------------------- defs
def defs():
    emit("<defs>")
    emit(f"<style>{svgkit.font_faces()}</style>")
    # brushed aluminium: horizontal streak noise mixed into the source graphic
    emit("""
<filter id="brushed" x="0" y="0" width="1" height="1" filterUnits="objectBoundingBox" color-interpolation-filters="sRGB">
  <feTurbulence type="fractalNoise" baseFrequency="0.0018 0.42" numOctaves="3" seed="7" result="n1"/>
  <feTurbulence type="fractalNoise" baseFrequency="0.02 0.9" numOctaves="2" seed="3" result="n2"/>
  <feComposite in="n1" in2="n2" operator="arithmetic" k2="0.6" k3="0.4" result="n"/>
  <feColorMatrix in="n" type="matrix" values="0.33 0.33 0.33 0 0  0.33 0.33 0.33 0 0  0.33 0.33 0.33 0 0  0 0 0 0 1" result="g"/>
  <feComposite in="SourceGraphic" in2="g" operator="arithmetic" k2="1" k3="0.46" k4="-0.23" result="mix"/>
  <feComposite in="mix" in2="SourceGraphic" operator="in"/>
</filter>
<filter id="soft" x="-20%" y="-20%" width="140%" height="160%"><feGaussianBlur stdDeviation="4"/></filter>
<filter id="soft2" x="-20%" y="-20%" width="140%" height="160%"><feGaussianBlur stdDeviation="1.6"/></filter>
<filter id="lcdshadow" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur stdDeviation="1.4"/></filter>
<linearGradient id="aluLight" x1="0" y1="0" x2="0.35" y2="1">
  <stop offset="0" stop-color="#ffffff" stop-opacity="0.18"/>
  <stop offset="0.5" stop-color="#ffffff" stop-opacity="0"/>
  <stop offset="1" stop-color="#000000" stop-opacity="0.10"/>
</linearGradient>
<linearGradient id="bezelGrad" x1="0" y1="0" x2="0" y2="1">
  <stop offset="0" stop-color="{bz[0]}"/>
  <stop offset="0.2" stop-color="{bz[1]}"/>
  <stop offset="1" stop-color="{bz[2]}"/>
</linearGradient>
<linearGradient id="bezelRim" x1="0" y1="0" x2="0" y2="1">
  <stop offset="0" stop-color="#e4e8ec"/>
  <stop offset="1" stop-color="#7c8188"/>
</linearGradient>
<linearGradient id="lcdGrad" x1="0" y1="0" x2="1" y2="0.3">
  <stop offset="0" stop-color="{lc[0]}"/>
  <stop offset="0.55" stop-color="{lc[1]}"/>
  <stop offset="1" stop-color="{lc[2]}"/>
</linearGradient>
<linearGradient id="glassGrad" x1="0" y1="0" x2="0" y2="1">
  <stop offset="0" stop-color="#5c6566"/>
  <stop offset="1" stop-color="#76807f"/>
</linearGradient>
<linearGradient id="capShade" x1="0" y1="0" x2="1" y2="0">
  <stop offset="0" stop-color="#ffffff" stop-opacity="0.10"/>
  <stop offset="0.75" stop-color="#000000" stop-opacity="0"/>
  <stop offset="1" stop-color="#000000" stop-opacity="0.18"/>
</linearGradient>
<linearGradient id="switchGrad" x1="0" y1="0" x2="0" y2="1">
  <stop offset="0" stop-color="#3a3a3a"/>
  <stop offset="1" stop-color="#151515"/>
</linearGradient>
<clipPath id="bodyClip"><rect x="20" y="24" width="2532" height="1072" rx="22"/></clipPath>
<filter id="brushedCap" x="0" y="0" width="1" height="1" filterUnits="objectBoundingBox" color-interpolation-filters="sRGB">
  <feTurbulence type="fractalNoise" baseFrequency="0.006 0.55" numOctaves="2" seed="11" result="n"/>
  <feColorMatrix in="n" type="matrix" values="0.33 0.33 0.33 0 0  0.33 0.33 0.33 0 0  0.33 0.33 0.33 0 0  0 0 0 0 1" result="g"/>
  <feComposite in="SourceGraphic" in2="g" operator="arithmetic" k2="1" k3="0.55" k4="-0.27" result="mix"/>
  <feComposite in="mix" in2="SourceGraphic" operator="in"/>
</filter>
""".replace("{bz[0]}", M["bezel"][0]).replace("{bz[1]}", M["bezel"][1]).replace("{bz[2]}", M["bezel"][2])
       .replace("{lc[0]}", M["lcd"][0]).replace("{lc[1]}", M["lcd"][1]).replace("{lc[2]}", M["lcd"][2]))
    emit("</defs>")


# ---------------------------------------------------------------- body & prints
def body_and_prints():
    layer("case")
    # dark plastic lower case visible under the plate
    emit('<rect x="24" y="1060" width="2524" height="58" rx="14" fill="#141414"/>')
    emit('<rect x="24" y="1060" width="2524" height="58" rx="14" fill="none" stroke="#2c2c2c" stroke-width="2"/>')
    end()

    # body + printed areas inside one filtered group -> brushing goes through ink
    emit('<g id="brushed-plate" filter="url(#brushed)">')
    layer("body")
    emit(f'<rect id="plate" x="20" y="24" width="2532" height="1072" rx="22" fill="{ALU}"/>')
    end()
    layer("print-areas")
    if M["fin"]:
        emit(f'<rect id="power-panel" x="72" y="62" width="182" height="368" fill="{FIN_PANEL}"/>')
        emit(f'<rect id="fin-panel" x="1606" y="68" width="894" height="358" fill="{FIN_PANEL}"/>')
    if M["panels"]:
        # power panel
        emit(f'<rect id="power-panel" x="72" y="62" width="182" height="368" fill="{PANEL}"/>')
        # keyboard BASIC panel (L-shape)
        emit(f'<path id="basic-panel" d="M100,779 H1597 V910 H1298 V1051 H100 Z" fill="{PANEL}" fill-opacity="0.75"/>')
    # right-hand grey bands (with a divider line they start right next to it)
    x0 = 1606 if M["divider"] else 1615
    if not M["fin"]:
        emit(f'<rect id="band1" x="{x0}" y="68" width="{2500 - x0}" height="119" fill="{BAND}"/>')
        emit(f'<rect id="band2" x="{x0}" y="193" width="{2352 - x0}" height="112" fill="{BAND}"/>')
        emit(f'<path id="band3" d="M{x0},311 H2500 V382 H2205 V426 H{x0} Z" fill="{BAND}"/>')
    end()
    emit("</g>")

    layer("lighting")
    emit('<rect x="20" y="24" width="2532" height="1072" rx="22" fill="url(#aluLight)"/>')
    # bevelled plate edge
    emit('<rect x="21" y="25" width="2530" height="1070" rx="21" fill="none" stroke="#eef1f4" stroke-opacity="0.7" stroke-width="2"/>')
    emit(f'<rect x="20" y="24" width="2532" height="1072" rx="22" fill="none" stroke="{ALU_EDGE}" stroke-width="1.5"/>')
    end()

    layer("print-areas-solid")
    if M["matrix"]:
        emit(f'<rect id="matrix-bar" x="703" y="503" width="296" height="37" fill="{BAR_RED}"/>')
    if M["fin"]:
        emit(f'<rect id="comp-box" x="1604" y="68" width="154" height="122" fill="{FIN_BOX}"/>')
    else:
        emit(f'<rect id="stat-bar" x="2355" y="193" width="145" height="24" fill="{M["stat_bar"]}"/>')
    end()

    layer("print-lines")
    s = f'fill="none" stroke="{BLACK_PRINT}" stroke-width="2.4"'
    # MODE frame with gap for the word
    emit(f'<path {s} d="M196,541 H100 V642 H397 V541 H300"/>')
    if M["matrix"]:
        emit(f'<path {s} d="M703,503 V642 H999 V503"/>')
    if M["divider"]:
        # BASIC outline is open to the right; the divider (interrupted where the outline meets it) closes it
        emit(f'<path {s} d="M1600,779 H100 V1051 H1298 V910 H1600"/>')
        emit(f'<path id="divider" {s} d="M1604,70 V774 M1604,784 V905 M1604,915 V1052"/>')
    else:
        emit(f'<path {s} d="M100,779 H1597 V910 H1298 V1051 H100 Z"/>')
    if M["fin"]:
        # financial key frame, separators between the financial keys, CD/(x,y)/DATA column
        emit(f'<path id="fin-frame" {s} d="M1604,68 H2352 V308 H2058 V426 H1604 Z"/>')
        seps = "".join(f"M{x},{RG_Y[1] - 12} V{RG_Y[1] + KH} " for x in (1760.5, 1909.5, 2058, 2205.5))
        seps += "".join(f"M{x},{RG_Y[2] - 12} V{RG_Y[2] + KH} " for x in (1760.5, 1909.5))
        emit(f'<path id="fin-separators" fill="none" stroke="{BLACK_PRINT}" stroke-width="1.8" d="{seps.strip()}"/>')
        emit(f'<path {s} d="M2320,433 H2500 V900 H2320 Z M2320,590 H2500 M2320,746 H2500"/>')
        end()
        return
    # STAT frame + right column
    emit(f'<path {s} d="M2355,217 V305 H2500"/>')
    emit(f'<path {s} d="M2500,193 V930 H2320 V465"/>')
    emit(f'<path {s} d="M2205,382 H2229 M2335,382 H2377 M2484,382 H2506"/>')
    emit(f'<path {s} d="M2205,382 V465 H2500 M2352,382 V465"/>')
    emit(f'<path {s} d="M2320,620 H2500 M2320,776 H2500"/>')
    end()


# ---------------------------------------------------------------- keys
LEFT_X = [114, 265, 415, 566, 716, 867, 1017, 1167, 1317, 1467]
LEFT_Y = [557, 692, 827, 962]
RG_X = [1630, 1779, 1928, 2076, 2223, 2370]
RG_Y = [106, 225, 345]
BR_X = [1631, 1810, 1988, 2165, 2341]
BR_Y = [471, 628, 784, 940]
KW, KH = 112, 75
BW, BH = 140, 105


def cap_geom(x, y, w, h):
    # fixed margins (not proportional) so wide keys like ENTER get the same black rim
    big = h > 90
    ml, mr = (7.7, 15.4) if big else (4.5, 14.0)
    cx = x + ml
    cy = y + h * (0.07 if big else 0.06)
    cw = w - ml - mr
    ch = h * 0.83
    return cx, cy, cw, ch


def key(ident, x, y, w, h, scheme, label, size=34, family=FONT_SANS, weight=600, fit=None, label_color=None,
        lpos=None, medium=False):
    scheme = {"grey": M["grey_keys"], "red": M["cce"]}.get(scheme, scheme)  # model-specific cap colours
    face, face_top, skirt, lcol = CAPS[scheme]
    lcol = label_color or lcol
    cx, cy, cw, ch = cap_geom(x, y, w, h)
    emit(f'<g id="key-{ident}" class="key" data-x="{x}" data-y="{y}" data-w="{w}" data-h="{h}">')
    # drop shadow on the plate
    emit(f'<rect x="{x + 4}" y="{y + 6}" width="{w}" height="{h}" rx="9" fill="#000" fill-opacity="0.35" filter="url(#soft)"/>')
    # well
    emit(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="7" fill="#0d0c0b"/>')
    emit(f'<rect x="{x + 0.8}" y="{y + 0.8}" width="{w - 1.6}" height="{h - 1.6}" rx="6.5" fill="none" stroke="#ffffff" stroke-opacity="0.10" stroke-width="1.4"/>')
    # cap body shadow inside well (cap side towards bottom/right)
    emit(f'<rect x="{cx + 2}" y="{cy + 3}" width="{cw + 4}" height="{ch + 6}" rx="5" fill="#000" fill-opacity="0.6" filter="url(#soft2)"/>')
    # skirt (front face of cap)
    emit(f'<rect x="{cx:.1f}" y="{cy:.1f}" width="{cw:.1f}" height="{ch:.1f}" rx="4" fill="{skirt}"/>')
    # top face
    fh = ch * 0.90
    brushed = ' filter="url(#brushedCap)"' if scheme == "metal" else ""
    emit(f'<rect x="{cx:.1f}" y="{cy:.1f}" width="{cw:.1f}" height="{fh:.1f}" rx="4" fill="{face}"{brushed}/>')
    if scheme == "brown":
        emit(f'<rect x="{cx:.1f}" y="{cy:.1f}" width="{cw:.1f}" height="{ch*0.10:.1f}" rx="3" fill="{face_top}"/>')
    else:
        emit(f'<rect x="{cx + 1:.1f}" y="{cy + 0.8:.1f}" width="{cw - 2:.1f}" height="{ch*0.08:.1f}" rx="3" fill="{face_top}"/>')
    emit(f'<rect x="{cx:.1f}" y="{cy:.1f}" width="{cw:.1f}" height="{fh:.1f}" rx="4" fill="url(#capShade)"/>')
    emit(f'<line x1="{cx + 3:.1f}" y1="{cy + 0.8:.1f}" x2="{cx + cw - 3:.1f}" y2="{cy + 0.8:.1f}" stroke="#fff" stroke-opacity="0.35" stroke-width="1.2"/>')
    # label
    lx, ly = cx + cw / 2, cy + fh / 2
    if lpos:
        lx, ly = lx + lpos[0], ly + lpos[1]
    if isinstance(label, tuple) and label[0] == "icon":
        icon(label[1], lx, ly, size, lcol)
    elif label is not None:
        rich(lx, ly, label, size, color=lcol, family=family, weight=weight, fit=fit, vcap=True,
             style=f"stroke:{lcol};stroke-width:0.9;stroke-linejoin:round" if medium else "")
    emit("</g>")


def keys():
    layer("keys")
    C = dict(family=FONT_COND, weight=700)                  # bold condensed (SML, C-CE)
    CM = dict(family=FONT_COND, weight=400, medium=True)    # medium condensed (CAL, BASIC, ...)
    # --- left block, row 0
    r0 = [("cal", "green", "CAL", CM), ("basic", "green", "BASIC", dict(CM, fit=80)), ("brk", "grey", "BRK", CM),
          ("def", "grey", "DEF", CM), ("down", "grey", ("icon", "down"), {}), ("up", "grey", ("icon", "up"), {}),
          ("left", "grey", ("icon", "left"), {}), ("right", "grey", ("icon", "right"), {}),
          ("sml", "grey", "SML", C), ("shift", "yellow", "SHIFT", dict(CM, fit=80))]
    for i, (n, sch, lab, kw) in enumerate(r0):
        if n == "sml" and not M["sml"]:
            continue
        key(n, LEFT_X[i], LEFT_Y[0], KW, KH, sch, lab, size=37 if kw else 26, **kw)
    for i, ch in enumerate("QWERTYUIOP"):
        key(ch.lower(), LEFT_X[i], LEFT_Y[1], KW, KH, "grey", ch, size=36)
    for i, ch in enumerate("ASDFGHJKL,"):
        key("comma" if ch == "," else ch.lower(), LEFT_X[i], LEFT_Y[2], KW, KH, "grey", ch, size=36 if ch != "," else 44)
    for i, ch in enumerate("ZXCVBNM"):
        key(ch.lower(), LEFT_X[i], LEFT_Y[3], KW, KH, "grey", ch, size=36)
    key("spc", LEFT_X[7], LEFT_Y[3], KW, KH, "grey", "SPC", size=38, **CM)
    key("enter", 1317, 960, 262, KH, "grey", "ENTER", size=38, **CM)

    # --- right grey keys
    rg = [
        [("hyp", "grey", "hyp", {}), ("sin", "grey", "sin", {}), ("cos", "grey", "cos", {}),
         ("tan", "grey", "tan", {}), ("fse", "grey", M["fse"], {}), ("cce", "red", "C·CE", C)],
        [("hex", "grey", [("→", "sab"), ("HEX", "c")], {"weight": 700, "size": 31}), ("deg", "grey", [("→", "sab"), ("DEG", "c")], {"weight": 700, "size": 31}),
         ("ln", "grey", "ln", {}), ("log", "grey", "log", {}),
         ("recip", "grey", [("1/", ""), ("x", "ib")], {}), ("stat", "grey", ("icon", "updown"), {})],
        [("exp", "grey", "EXP", {}), ("pow", "grey", [("y", "ib"), ("x", "ib^")], {}), ("sqrt", "grey", ("icon", "sqrt"), {}),
         ("sq", "grey", [("x", "ib"), ("2", "^")], {}), ("lpar", "grey", "(", {}), ("rpar", "grey", ")", {})],
    ]
    if M["fin"]:
        P = [("P", "c"), ("1", "_c"), ("/", "c"), ("P", "c"), ("2", "_c")]
        rg = [
            [("comp", "grey", "COMP", C), ("amrt", "grey", "AMRT", C), ("p1p2", "grey", P, C),
             ("acc", "grey", "ACC", C), ("cfi", "grey", [("CF", ""), ("i", "ib")], {}), ("cce", "red", "C·CE", C)],
            [("fin-n", "grey", "n", {}), ("fin-i", "grey", "i", {}), ("pv", "grey", "PV", C),
             ("fv", "grey", "FV", C), ("pmt", "grey", "PMT", C), ("pct", "grey", "%", {})],
            [("cst", "grey", "CST", C), ("sel", "grey", "SEL", C), ("mar", "grey", "MAR", C),
             ("pow", "grey", [("y", "ib"), ("x", "ib^")], {}), ("recip", "grey", [("1/", ""), ("x", "ib")], {}),
             ("sqrt", "grey", ("icon", "sqrt"), {})],
        ]
    for r, row in enumerate(rg):
        for i, (n, sch, lab, kw) in enumerate(row):
            kw = dict(kw)
            size = 36 if kw.get("family") != FONT_COND else 40
            if n in ("hex", "deg"):
                size = kw.pop("size")
            if isinstance(lab, tuple):
                size = 24
            if n in ("comp", "amrt", "p1p2", "pmt"):
                kw["fit"] = 78
            kw.setdefault("weight", 700)
            key(n, RG_X[i], RG_Y[r], KW, KH, sch, lab, size=size, **kw)

    # --- brown keys
    br = [
        [("7", "7"), ("8", "8"), ("9", "9"), ("div", [("÷", ""), ("/", "")]), ("xm", [("X", "i"), ("→", "sab"), ("M", "b")])],
        [("4", "4"), ("5", "5"), ("6", "6"), ("mul", [("×", ""), ("∗", "")]), ("rm", "RM(" if M["fin"] else "RM")],
        [("1", "1"), ("2", "2"), ("3", "3"), ("minus", "−"), ("mplus", "M+)" if M["fin"] else "M+")],
        [("0", "0"), ("sign", "+/−"), ("dot", ("icon", "dot")), ("plus", "+"), ("eq", "=")],
    ]
    for r, row in enumerate(br):
        for i, (n, lab) in enumerate(row):
            col = BLUE_LABEL if i == 4 and r < 3 else None
            size = 44
            if n in ("xm", "rm", "mplus"):
                size = 40
            if n in ("minus", "plus", "eq", "mul", "div", "sign"):
                size = 54
            if isinstance(lab, tuple):
                size = 24
            key(n, BR_X[i], BR_Y[r], BW, BH, "brown", lab, size=size, family=FONT_GEO, weight=700, label_color=col)
    end()


# ---------------------------------------------------------------- printed text
def label_pos(x, y, w, dy):
    cx, _, cw, _ = cap_geom(x, y, w, KH)
    return cx + cw / 2, y + dy


def printed_text():
    layer("print-text")
    R, K = RED_PRINT, BLACK_PRINT
    # power area
    for t, yy in (("POWER", 160), ("ON", 222), ("OFF", 365)):
        rich(165, yy, t, 27, color="#2a2a2a", weight=400)
    # row 0 captions
    rich(248, 541, "MODE", 27, color=K, weight=400)
    rich(*label_pos(LEFT_X[2], LEFT_Y[0], KW, -19), "ON", 27, color=K, weight=400)
    if M["matrix"]:
        rich(851, 521, "MATRIX", 28, color="#f4f0ec", weight=500)
    rich(*label_pos(LEFT_X[6], LEFT_Y[0], KW, -19), "DEL", 27, color=K, weight=400)
    rich(*label_pos(LEFT_X[7], LEFT_Y[0], KW, -19), "INS", 27, color=K, weight=400)
    # row 1 shift symbols
    for i, t in enumerate(['!', '"', '#', '$', '%', '&', '?', '@', ':', ';']):
        rich(*label_pos(LEFT_X[i], LEFT_Y[1], KW, -21 + (7 if t == '"' else 0)), t, 29 if t != '"' else 40, color=R, weight=400)
    for i, t in enumerate(["INPUT", "IF", "THEN", "GOTO", "FOR", "TO", "STEP", "NEXT", "LIST", "RUN"]):
        rich(*label_pos(LEFT_X[i], LEFT_Y[2], KW, -18), t, 27, color=R, weight=400)
    for i, t in enumerate(["PRINT", "USING", "GOSUB", "RETURN", "DIM", "END", "CSAVE", "CLOAD"]):
        rich(*label_pos(LEFT_X[i], LEFT_Y[3], KW, -18), t, 27, color=R, weight=400)
    # P<->NP
    rich(1410, 942, "P", 27, color=R, weight=400)
    rich(1478, 942, "NP", 27, color=R, weight=400)
    emit(f'<path d="M1424,942 h28" stroke="{R}" stroke-width="2.6"/>'
         f'<path d="M1421,942 l8,-6 v12 z M1455,942 l-8,-6 v12 z" fill="{R}"/>')

    if M["fin"]:
        fin_text()
        end()
        return
    # right gray rows
    rows = [
        [[("archyp", "")], [("sin", ""), ("−1", "^")], [("cos", ""), ("−1", "^")], [("tan", ""), ("−1", "^")],
         [("TAB", "")], [("CA", "")]],
        [[("→", "sab"), ("DEC", "")], [("→", "sab"), ("D.MS", "")], [("e", "i"), ("x", "i^"), (" E", "k")], [("10", ""), ("x", "i^"), (" F", "k")],
         [("→", "sab"), ("rθ", "i")], None],
        [[("π", "i"), ("  A", "k")], [("x", "i^"), ("√", ""), ("y", "i"), ("  B", "k")], [("3", "^"), ("√", ""), ("  C", "k")],
         [("Δ%", ""), (" D", "k")], [("→", "sab"), ("xy", "i")], [("n", "i"), ("!", "")]],
    ]
    for r, row in enumerate(rows):
        for i, segs in enumerate(row):
            if segs is None:
                continue
            x, y = label_pos(RG_X[i], RG_Y[r], KW, -16)
            rich(x, y, segs, 28, color=R, weight=400)
    rich(2427, 205, "STAT", 23, color="#f4f0ec", weight=500)
    # STAT sub labels
    rich(2232, 444, "n", 28, color=K, weight=400)
    rich(2300, 444, [("Σ", ""), ("xy", "i")], 28, color=R, weight=400)
    rich(2388, 444, [("Σ", ""), ("x", "i")], 28, color=K, weight=400)
    rich(2466, 444, [("Σ", ""), ("x", "i"), ("2", "^")], 28, color=R, weight=400)
    rich(2376, 600, [("Σ", ""), ("y", "i")], 28, color=K, weight=400)
    rich(2464, 600, [("Σ", ""), ("y", "i"), ("2", "^")], 28, color=R, weight=400)
    rich(2412, 757, [("(", ""), ("x", "i"), (", ", ""), ("y", "i"), (")", "")], 28, color=K, weight=500)
    rich(2369, 913, "DATA", 25, color=K, weight=500)
    rich(2470, 913, "CD", 25, color=R, weight=400)

    # brown key shift labels
    brl = [
        [[("y", "i")], [("s", ""), ("y", "i")], [("σ", "i"), ("y", "i")], None],
        [[("x", "i")], [("s", ""), ("x", "i")], [("σ", "i"), ("x", "i")], [("<", "")]],
        [[("r", "")], [("a", "")], [("b", "")], [(">", "")]],
        [[("x", "i"), ("′", "g")], [("y", "i"), ("′", "g")], [("DRG", "")], [("∧", "g")]],
    ]
    for r, row in enumerate(brl):
        for i, segs in enumerate(row):
            if segs is None:
                continue
            cx, _, cw, _ = cap_geom(BR_X[i], BR_Y[r], BW, BH)
            x, y = cx + cw / 2, BR_Y[r] - 20
            rich(x, y, segs, 28, color=R, weight=400)
            if r < 2 and i == 0:  # mean bar over x / y
                emit(f'<line x1="{x - 7:.1f}" y1="{y - 15:.1f}" x2="{x + 7:.1f}" y2="{y - 15:.1f}" stroke="{R}" stroke-width="2"/>')
    end()


def fin_text():
    """PC-1421: shift labels of the financial keys, statistics labels on the brown keys, DAYS line."""
    R = RED_PRINT
    rows = [
        [None, [("STO", "")], [("NPV", "")], [("IRR", "")], [("N", ""), ("i", "i")], [("CA", "")]],
        [[("×12", "")], [("÷12", "")], [("→", "sab"), ("APR", "")], [("→", "sab"), ("EFF", "")],
         [("n", "i"), ("!", "")], [("Δ%", "")]],
        [[("BGN", "")], [("TAB", "")], [("MU", "")], [("e", "i"), ("x", "i^")], [("ln", "")],
         [("x", "i"), ("2", "^")]],
    ]
    for r, row in enumerate(rows):
        for i, segs in enumerate(row):
            if segs is not None:
                rich(*label_pos(RG_X[i], RG_Y[r], KW, -16), segs, 28, color=R, weight=400)
    rich(*label_pos(RG_X[0], RG_Y[0], KW, -16), "RCL", 28, color=FIN_ORANGE, weight=400)
    brl = [
        [[("y", "i")], [("s", ""), ("y", "i")], [("σ", "i"), ("y", "i")], [("r", "")], [("CD", "k")]],
        [[("x", "i")], [("s", ""), ("x", "i")], [("σ", "i"), ("x", "i")], [("a <", "")],
         [("(", "k"), ("x", "ik"), (", ", "k"), ("y", "ik"), (")", "k")]],
        [[("Σ", ""), ("xy", "i")], [("Σ", ""), ("y", "i")], [("Σ", ""), ("y", "i"), ("2", "^")], [("b >", "")],
         [("DATA", "k")]],
        [[("n", "i")], [("Σ", ""), ("x", "i")], [("Σ", ""), ("x", "i"), ("2", "^")],
         [("x", "i"), ("′", "g"), (" ∧", "g")], [("y", "i"), ("′", "g")]],
    ]
    for r, row in enumerate(brl):
        for i, segs in enumerate(row):
            cx, _, cw, _ = cap_geom(BR_X[i], BR_Y[r], BW, BH)
            x, y = cx + cw / 2, BR_Y[r] - 20
            rich(x, y, segs, 28, color=R, weight=400)
            if r < 2 and i == 0:  # mean bar over x / y
                emit(f'<line x1="{x - 7:.1f}" y1="{y - 15:.1f}" x2="{x + 7:.1f}" y2="{y - 15:.1f}" stroke="{R}" stroke-width="2"/>')
    # day count basis under the bottom row: I(360) -- II(365) -- [DAYS]
    y = 1068
    rich(1703, y, "I(360)", 24, color=R, weight=400)
    rich(1878, y, "II(365)", 24, color=R, weight=400)
    emit(f'<path d="M1746,{y} H1832 M1922,{y} H1962" stroke="{R}" stroke-width="2.2"/>')
    emit(f'<rect x="1964" y="{y - 13}" width="64" height="26" fill="{R}"/>')
    rich(1996, y, "DAYS", 21, color="#f4f0ec", weight=500, fit=54)


# ---------------------------------------------------------------- display
LCD = dict(x=488, y=195, w=878, h=187)
CHARS = 24
DOT_PX, DOT_PY = 6.08, 10.75          # dot pitch
DOT_W, DOT_H = 5.0, 9.3               # dot size
TEXT_X0, TEXT_Y0 = 492, 255           # top-left of first character cell


def _bx(xb):  # x on a close-up of the display -> panel x (via character cells: 190 -> 484, pitch 93.3 -> 36.5)
    return 484 + (xb - 190) * 0.391


SEG_TOP_Y = 222
SEG_TOP = {n: (_bx(a), _bx(b)) for n, a, b in (
    ("BUSY", 225, 385), ("DEF", 400, 525), ("SHIFT", 690, 850), ("HYP", 865, 1000),
    ("KANA", 1290, 1370), ("SMALL", 1395, 1470), ("SML", 1490, 1630), ("DEG", 1650, 1755),
    ("RAD", 1760, 1900), ("(", 2150, 2178), (")", 2212, 2240), ("M", 2285, 2333), ("E", 2345, 2385))}
# mode bars: unlabeled ones from the close-up, labeled ones directly above their printed ticks
SEG_BARS = {"BAR1": 507, "CAL": 583, "RUN": 647, "PRO": 712, "BAR5": 776}  # + M["glass"] bars
SEG_BOX = {}


def display(text, cursor, segments):
    layer("display")
    # metallic rim + bezel
    emit('<rect x="254" y="43" width="1312" height="458" rx="12" fill="url(#bezelRim)"/>')
    emit('<rect x="261" y="49" width="1298" height="446" rx="8" fill="url(#bezelGrad)"/>')
    emit(f'<rect x="275" y="138" width="1281" height="3.2" fill="{M["logo_rule"]}"/>')
    # logo + model
    rich(525, 104, M["name"], 50, color="#f1f1ef", family=FONT_LOGO, weight=700, fit=330)
    if M["subtitle"]:
        rich(735, 106, M["subtitle"], 36, color="#f1f1ef", weight=400, anchor="start")
    if M["badge"]:  # memory badge, e.g. 32KB
        big, small, width = M["badge"]
        ink = "#e9873b"
        emit(f'<g id="badge-{big}{small.strip(".")}kb">')
        # digits: dark on an orange block as tall as the KB frame, ending just left of it
        emit(f'<rect x="{1398 - width - 8}" y="86.8" width="{width + 8}" height="37.4" fill="{ink}"/>')
        rich(1394, 122 - 0.35 * 44, [(big, "")] + ([(small, "s")] if small else []), 44, color=M["bezel"][2],
             family=FONT_GEO, weight=700, anchor="end", fit=width - 4,
             style=f"stroke:{M['bezel'][2]};stroke-width:1", vcap=True)
        # KB: orange on the bezel, in an orange frame
        emit(f'<rect x="1401" y="88" width="69" height="35" rx="3" fill="none" stroke="{ink}" stroke-width="2.4"/>')
        rich(1435.5, 105.5, "KB", 31, color=ink, weight=700, fit=52, vcap=True)
        emit("</g>")
    rich(1469, 468, "MEMORY SAFE GUARD / AUTO POWER OFF", 21, color="#f4f4f2", weight=600, anchor="end", fit=455)

    # window
    emit('<rect x="355" y="150" width="1112" height="298" rx="9" fill="#0d0d0d"/>')
    emit('<rect x="396" y="183" width="1052" height="258" rx="3" fill="url(#glassGrad)"/>')
    emit(f'<rect id="lcd-bg" x="{LCD["x"]}" y="{LCD["y"]}" width="{LCD["w"]}" height="{LCD["h"]}" fill="url(#lcdGrad)"/>')
    # printed annunciator captions on the glass
    if M["fin"]:
        for t, (xx, _, box) in FIN_GLASS.items():
            if box:
                emit(f'<rect x="{xx - box / 2}" y="390" width="{box}" height="27" fill="#141414"/>')
            rich(xx, 403, t, 24, color="#b7c0bf" if box else "#141414", weight=500)
        tick = 'stroke="#141414" stroke-width="2.4" fill="none"'
        emit(f'<path {tick} d="M665,390 V383 H719 V390 M692,383 V378 M1157,390 V383 H1220 V390 M1188,383 V378"/>')
        emit(f'<path {tick} d="M587,390 V378"/>')
    else:
        glass_captions()
    # glass reflection
    emit('<path d="M396,183 L1448,183 L1448,200 L396,212 Z" fill="#ffffff" fill-opacity="0.04"/>')
    dots(text, cursor, segments)


def glass_captions():
    rich(582, 403, "CAL", 24, color="#141414", weight=500)
    rich(646, 403, "RUN", 24, color="#141414", weight=500)
    rich(711, 403, "PRO", 24, color="#141414", weight=500)
    emit('<rect x="750" y="390" width="80" height="27" fill="#141414"/>')
    rich(790, 403, "BASIC", 24, color="#b7c0bf", weight=500)
    for t, (xx, _) in M["glass"].items():
        rich(xx, 403, t, 24, color="#141414", weight=500)
    tick = 'stroke="#141414" stroke-width="2.4" fill="none"'
    emit(f'<path {tick} d="M646,390 V383 H711 V390 M679,383 V378"/>')
    emit(f'<path {tick} d="M583,390 V378{"".join(f" M{bx},390 V378" for _, bx in M["glass"].values())}"/>')


def dots(text, cursor, segments):

    # dots
    emit('<g id="lcd-dots">')
    on, ghost = [], []
    for c in range(CHARS):
        ch = text[c] if c < len(text) else " "
        glyph = FONT.get(ch, FONT["?"])
        if c == cursor:
            glyph = glyph[:6] + ["11111"]
        for ry, row in enumerate(glyph):
            for rx, bit in enumerate(row):
                x = TEXT_X0 + (c * 6 + rx) * DOT_PX
                y = TEXT_Y0 + ry * DOT_PY
                (on if bit == "1" else ghost).append((x, y))
    def rects(pts, dx=0, dy=0):
        return "".join(f"M{x + dx:.2f},{y + dy:.2f}h{DOT_W}v{DOT_H}h-{DOT_W}z" for x, y in pts)
    emit(f'<path id="lcd-ghost" d="{rects(ghost)}" fill="#000" fill-opacity="0.010"/>')
    emit(f'<path id="lcd-dot-shadow" d="{rects(on, 2.2, 2.6)}" fill="#3d4520" fill-opacity="0.35" filter="url(#lcdshadow)"/>')
    emit(f'<path id="lcd-on" d="{rects(on)}" fill="#1d2218" fill-opacity="0.93"/>')
    emit("</g>")

    # status segments (positions mapped from an oblique close-up of the display via the
    # 24-cell character grid, which lies in the same plane as the segments)
    emit('<g id="lcd-segments">')
    ink = "#1d2218"
    for name, (x0, x1) in SEG_TOP.items():
        if name == "SML" and not M["sml"]:
            continue
        op = 0.92 if name in segments else 0.0
        cx, w = (x0 + x1) / 2, x1 - x0
        SEG_BOX[name] = dict(x=x0, y=SEG_TOP_Y - 10, w=w, h=20)
        emit(f'<g id="seg-{name}" class="segment" opacity="{op}">')
        if name == "M":  # inverted M
            emit(f'<rect x="{x0 - 2:.1f}" y="{SEG_TOP_Y - 11:.1f}" width="{w + 4:.1f}" height="22" fill="{ink}"/>')
            fill, txt = "#bcc37e", "M"
        else:
            fill, txt = ink, {"KANA": "カナ", "SMALL": "小"}.get(name, name)
        fam = "'Hiragino Sans', sans-serif" if name in ("KANA", "SMALL") else FONT_COND
        emit(f'<text x="{cx:.1f}" y="{SEG_TOP_Y + 8.5:.1f}" font-family="{fam}" font-size="24.5" font-weight="700" '
             f'fill="{fill}" text-anchor="middle" textLength="{w:.1f}" lengthAdjust="spacingAndGlyphs">{escape(txt)}</text>')
        emit("</g>")
    bars = ({n: bx for n, (_, bx, _) in FIN_GLASS.items() if bx} if M["fin"]
            else {**SEG_BARS, **{n: bx for n, (_, bx) in M["glass"].items()}})
    for name, bx in bars.items():
        op = 0.92 if name in segments else 0.0
        SEG_BOX[name] = dict(x=bx - 10, y=355, w=20, h=7)
        emit(f'<rect id="seg-{name}" class="segment" x="{bx - 10}" y="355" width="20" height="7" fill="{ink}" opacity="{op}"/>')
    emit("</g>")
    end()


def power_switch():
    layer("power-switch")
    emit('<rect x="113" y="246" width="88" height="96" rx="6" fill="#0c0c0c"/>')
    emit('<rect x="122" y="254" width="70" height="80" rx="4" fill="#1b1b1b"/>')
    emit('<rect x="124" y="256" width="66" height="36" rx="3" fill="url(#switchGrad)"/>')
    emit('<rect x="124" y="256" width="66" height="3" fill="#ffffff" fill-opacity="0.15"/>')
    end()


def main():
    global M
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", choices=MODELS, default="1403h")
    ap.add_argument("--text", default="", help='LCD text, e.g. \'10PRINT"Hello, World!"\' (default: LCD off)')
    ap.add_argument("--cursor", type=int, default=-1, help="cursor column (default: none)")
    ap.add_argument("--segments", default="", help="lit status segments, e.g. SML,DEG,PRO; 'model' for the model's own")
    ap.add_argument("--keep-svg", action="store_true", help="keep the SVG next to the PNG")
    ap.add_argument("--no-png", action="store_true", help="write the SVG and layout only")
    ap.add_argument("-o", "--out", required=True, help="SVG path (PNG and layout JSON next to it)")
    a = ap.parse_args()
    M = MODELS[a.model]
    a.segments = M["segments"] if a.segments == "model" else a.segments
    if not M["lowercase"]:
        a.text = a.text.upper()
    svgkit.out.clear()
    emit(svgkit.svg_open(W, H))
    defs()
    body_and_prints()
    printed_text()
    display(a.text, a.cursor, set(a.segments.split(",")) - {""})
    power_switch()
    keys()
    emit("</svg>")
    svg = "\n".join(svgkit.out)
    lcd = dict(LCD, text_x0=TEXT_X0, text_y0=TEXT_Y0, dot_pitch_x=DOT_PX, dot_pitch_y=DOT_PY,
               dot_w=DOT_W, dot_h=DOT_H, chars=CHARS)
    svgkit.finish(a.out, svg, {"size": [W, H], "keys": svgkit.keymap(svg), "lcd": lcd, "segments": SEG_BOX},
                  a.keep_svg, a.no_png)


if __name__ == "__main__":
    main()
