#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10,<3.15"
# dependencies = ["fonttools", "pyoxipng"]
# ///
"""Generate the layered, semantic SVGs of the three Calc-U-59 cue card templates.

Coordinate system: 2064 x 440 units, the size of the app's card window kCardRect (CueCard,
MagnetCard, SolidStateCard: the card art the app draws a fitted card on). The geometry was
measured on the app's reference card images and made exact: straight rules of one width,
regular column pitch, one corner radius per card. The cells line up with the app's text layout
(app/CueCard.cpp cueCardLayout: xFractions, title/grid rows); the MagnetCard arrows are the
app's own points (cardCornerGeometry), as the app redraws them on a lit corner.
Fonts, font metrics, the output buffer, rich text and the rendering are in tools/faceplate/svgkit.py.

Layers of each card (bottom to top), inside a group clipped to the card's outline:
  card    the card itself (rounded rectangle)
  print   printed areas (MagnetCard: top band, badges, right strip), rules, column dividers,
          the right column, the arrows, the SolidState title (as outlines: no font needed)

Usage: build_cards.py [--keep-svg] [--no-png] -o DIR
Writes <DIR>/CueCard.svg, MagnetCard.svg, SolidStateCard.svg and the PNGs next to them (rendered
with headless Chrome, losslessly optimised with oxipng; the SVGs are deleted unless --keep-svg).
"""
import argparse
import sys
from pathlib import Path

from fontTools.pens.boundsPen import BoundsPen
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen

HERE = Path(__file__).resolve().parent   # faceplate/
sys.dont_write_bytecode = True
sys.path.insert(0, str(HERE.parents[2] / "tools" / "faceplate"))
import svgkit  # noqa: E402  (fonts, output buffer, rendering)
from svgkit import emit, end, layer  # noqa: E402
W, H = 2064, 440
RULE = 8   # every printed rule and divider


def card_path(y0, y1, r):
    """Rounded card outline from y0 to y1, radius r."""
    return (f"M{r},{y0} H{W - r} A{r},{r} 0 0 1 {W},{y0 + r} V{y1 - r} A{r},{r} 0 0 1 {W - r},{y1} "
            f"H{r} A{r},{r} 0 0 1 0,{y1 - r} V{y0 + r} A{r},{r} 0 0 1 {r},{y0} Z")


def hrule(y, x0, x1, color):
    """A rule centred on y."""
    return f'<rect x="{x0}" y="{y - RULE / 2}" width="{x1 - x0}" height="{RULE}" fill="{color}"/>'


def vrule(x, y0, y1, color):
    """A rule centred on x."""
    return f'<rect x="{x - RULE / 2}" y="{y0}" width="{RULE}" height="{y1 - y0}" fill="{color}"/>'


def svg(name, card, layers):
    svgkit.out.clear()
    emit(svgkit.svg_open(W, H))
    emit(f"<title>{name}</title>")
    emit(f'<defs><clipPath id="cardClip"><path d="{card}"/></clipPath></defs>')
    emit('<g clip-path="url(#cardClip)">')
    for ident, content in layers:
        layer(ident)
        emit(content)
        end()
    emit("</g>")
    emit("</svg>")
    return "\n".join(svgkit.out) + "\n"


def columns(first, pitch):
    """x of the four column dividers."""
    return [first + i * pitch for i in range(4)]


# ---------------------------------------------------------------- CueCard: the blank card, written by hand
def cue_card():
    gold, ink = "#c49223", "#382105"
    card = card_path(1, 439, 64)
    rows = (128.5, 231.5, 333.5)          # under the top margin (wash), under the title, between the label rows
    right = 2027.5                        # the right column, from the top rule down
    lines = [hrule(rows[0], 0, W, ink), hrule(rows[1], 0, right, ink), hrule(rows[2], 0, right, ink),
             vrule(right, rows[0], H, ink)]
    lines += [vrule(x, rows[1], H, ink) for x in columns(421, 401.7)]
    return card, [("card", f'<path d="{card}" fill="{gold}"/>'),
                  ("print", "\n".join(lines))]


# ---------------------------------------------------------------- MagnetCard: the TI-59 magnetic card
# The arrows: app/CueCard.cpp cardCornerGeometry kLeft / kRight (fractions of the card).
ARROW_L = [(0.1220, 0.1479), (0.1410, 0.0728), (0.1410, 0.1033), (0.1495, 0.1033), (0.1495, 0.1948),
           (0.1410, 0.1948), (0.1410, 0.2254)]
ARROW_R = [(0.8900, 0.1362), (0.8705, 0.0610), (0.8705, 0.0915), (0.8625, 0.0915), (0.8625, 0.1831),
           (0.8705, 0.1831), (0.8705, 0.2136)]


def magnet_card():
    gold, black = "#d2a92b", "#000000"
    card = card_path(0, 440, 36)
    band, strip = 123, 1982               # black top band (the card's magnetic strip side), black right strip
    rows = (226.5, 334)
    badges = [(78, 27, 250, 78), (1763, 22, 247, 78)]   # bank badges; the right one sits 5 higher, as its arrow
    areas = [f'<path d="{card}" fill="{black}"/>',
             f'<rect x="0" y="{band}" width="{strip}" height="{H - band}" fill="{gold}"/>']
    areas += [f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="6" fill="{gold}"/>' for x, y, w, h in badges]
    arrows = "".join("M" + " L".join(f"{fx * W:.1f},{fy * H:.1f}" for fx, fy in pts) + " Z" for pts in (ARROW_L, ARROW_R))
    lines = [hrule(y, 0, strip, black) for y in rows] + [vrule(x, rows[0], H, black) for x in columns(399.5, 396)]
    lines.append(f'<path id="arrows" d="{arrows}" fill="{black}"/>')
    return card, [("card", areas[0]), ("print", "\n".join(areas[1:] + lines))]


# ---------------------------------------------------------------- SolidStateCard: the module card, printed gold
def title_path(text, box):
    """The title as outlines (TeX Gyre Adventor bold ~ ITC Avant Garde) filling its ink box:
    sized by the height, stretched to the width."""
    font = svgkit.font("texgyreadventor-bold.otf")
    gs, cmap, hmtx = font.getGlyphSet(), font.getBestCmap(), font["hmtx"]
    x, glyphs = 0, []
    for ch in text:
        g = cmap[ord(ch)]
        glyphs.append((g, x))
        x += hmtx[g][0]
    bp = BoundsPen(gs)
    for g, gx in glyphs:
        gs[g].draw(TransformPen(bp, (1, 0, 0, 1, gx, 0)))
    x0, y0, x1, y1 = bp.bounds
    s = box[3] / (y1 - y0)                # the height decides the size ...
    sx = box[2] / (x1 - x0)               # ... the printed letters are wider: stretched to the width
    tx = box[0] - x0 * sx
    ty = box[1] + box[3] + y0 * s         # font y up -> SVG y down
    pen = SVGPathPen(gs)
    for g, gx in glyphs:
        gs[g].draw(TransformPen(pen, (sx, 0, 0, -s, tx + gx * sx, ty)))
    return pen.getCommands()


def solid_state_card():
    gold, black = "#bd9d5a", "#000000"
    card = card_path(1, 439, 64)
    rows = (106.5, 212.5, 316.5)
    right = 1977.5
    lines = [hrule(rows[0], 0, W, gold), hrule(rows[1], 0, right, gold), hrule(rows[2], 0, right, gold),
             vrule(right, rows[0], H, gold)]
    title = title_path("Solid State Software", (460, 26, 901, 55))
    lines.append(f'<path id="title" d="{title}" fill="{gold}"/>')
    return card, [("card", f'<path d="{card}" fill="{black}"/>'), ("print", "\n".join(lines))]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--keep-svg", action="store_true", help="keep the SVGs next to the PNGs")
    ap.add_argument("--no-png", action="store_true", help="write the SVGs only (implies --keep-svg)")
    ap.add_argument("-o", "--out", required=True, help="output directory")
    a = ap.parse_args()
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    for name, make in (("CueCard", cue_card), ("MagnetCard", magnet_card), ("SolidStateCard", solid_state_card)):
        card, layers = make()
        path = out / f"{name}.svg"
        path.write_text(svg(name, card, layers))
        if not a.no_png:
            svgkit.render_png(path)
            if not a.keep_svg:
                path.unlink()


if __name__ == "__main__":
    main()
