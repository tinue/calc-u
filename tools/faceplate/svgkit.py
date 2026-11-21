"""SVG helpers shared by the faceplate generators of the products (products/*/faceplate/):
the embedded fonts and their metrics, the output buffer and layers, rich text, small vector
icons, the key rectangles of a generated SVG and the rendering to PNG.
"""
import base64
import io
from pathlib import Path
from xml.sax.saxutils import escape

HERE = Path(__file__).resolve().parent   # tools/faceplate/: render.sh
FONTS = HERE / "fonts"                   # TeX Gyre, GUST Font License

RED_PRINT = "#a9443a"
BLACK_PRINT = "#1e1e1e"

# Embedded TeX Gyre fonts (GUST Font License), see fonts/:
#   Heros    ~ Helvetica            printed labels, bold key legends
#   Heros Cn ~ Helvetica Condensed  narrow key legends (CAL, BASIC, ...)
#   Adventor ~ ITC Avant Garde      digits on the brown keys, logo text
#   Termes   ~ Times italic         math letters x, y, pi ...
FONT_SANS = "PCHeros, 'Helvetica Neue', Helvetica, sans-serif"
FONT_COND = "PCHerosCn, 'Helvetica Neue', sans-serif"
FONT_MATH = "PCTermes, 'Times New Roman', serif"
FONT_GEO = "PCAdventor, 'Avenir Next', sans-serif"
FONT_LOGO = FONT_GEO
EMBED = [  # family, file, weight, style
    ("PCHeros", "texgyreheros-regular.otf", 400, "normal"),
    ("PCHeros", "texgyreheros-bold.otf", 700, "normal"),
    ("PCHerosCn", "texgyreheroscn-regular.otf", 400, "normal"),
    ("PCHerosCn", "texgyreheroscn-bold.otf", 700, "normal"),
    ("PCAdventor", "texgyreadventor-bold.otf", 700, "normal"),
    ("PCTermes", "texgyretermes-italic.otf", 400, "italic"),
    ("PCTermes", "texgyretermes-bolditalic.otf", 700, "italic"),
]


def face(family, weight, style=None):
    """The EMBED entry (family, file, weight, style) of a font."""
    return next(e for e in EMBED if e[0] == family and e[2] == weight and (style is None or e[3] == style))


def font_faces(families=None):
    """@font-face rules with base64 subsets (printable ASCII + symbols used) of the embedded
    fonts, or of the given families only."""
    from fontTools import subset
    from fontTools.ttLib import TTFont
    text = "".join(chr(c) for c in range(32, 127)) + "ΣσθπΔ′∧÷×∗−–→↔·√x̄ȳ≥"
    rules = []
    for fam, fn, wt, st in EMBED:
        if families and fam not in families:
            continue
        font = TTFont(FONTS / fn)
        sub = subset.Subsetter(subset.Options())
        sub.populate(text=text)
        sub.subset(font)
        buf = io.BytesIO()
        font.save(buf)
        b64 = base64.b64encode(buf.getvalue()).decode()
        rules.append(f"@font-face{{font-family:{fam};font-weight:{wt};font-style:{st};"
                     f"src:url(data:font/otf;base64,{b64}) format('opentype');}}")
    return "\n".join(rules)


# ---------------------------------------------------------------- font metrics
_fonts, _bounds = {}, {}


def font(file):
    """The font file (in fonts/) as a fontTools TTFont, loaded once."""
    if file not in _fonts:
        from fontTools.ttLib import TTFont
        _fonts[file] = TTFont(FONTS / file)
    return _fonts[file]


def advance(text, file, size):
    """The advance width of a text at `size`."""
    f = font(file)
    cmap, hmtx, upm = f.getBestCmap(), f["hmtx"], f["head"].unitsPerEm
    return sum(hmtx[cmap[ord(c)]][0] for c in text) * size / upm


def glyph_runs(text, file, size):
    """(x offset, ink box) per glyph of a text at `size`; boxes y-up, None for blanks."""
    from fontTools.pens.boundsPen import BoundsPen
    f = font(file)
    cmap, hmtx, upm, gs = f.getBestCmap(), f["hmtx"], f["head"].unitsPerEm, f.getGlyphSet()
    runs, x = [], 0.0
    for ch in text:
        g = cmap[ord(ch)]
        if (file, g) not in _bounds:
            pen = BoundsPen(gs)
            gs[g].draw(pen)
            _bounds[file, g] = pen.bounds
        b = _bounds[file, g]
        runs.append((x, None if b is None else tuple(v * size / upm for v in b)))
        x += hmtx[g][0] * size / upm
    return runs


# ---------------------------------------------------------------- output buffer
out = []


def svg_open(w, h):
    """The <svg> start tag of a w x h canvas."""
    return (f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" '
            f'width="{w}" height="{h}" viewBox="0 0 {w} {h}">')


def emit(s):
    out.append(s)


def layer(name, extra=""):
    emit(f'<g id="{name}" inkscape:groupmode="layer" inkscape:label="{name}"{extra}>')


def end():
    emit("</g>")


# ---------------------------------------------------------------- rich text
def rich(x, y, segs, size, color=BLACK_PRINT, family=FONT_SANS, weight=500,
         anchor="middle", fit=None, style="", ident=None, vcap=False):
    """segs: str or list of (text, flags). flags chars:
    r=red k=black w=white i=italic-math ^=superscript _=subscript b=bold c=condensed
    """
    if isinstance(segs, str):
        segs = [(segs, "")]
    if vcap:  # centre the cap height on y (baseline = y + cap/2)
        y += 0.35 * size
    attrs = (f'x="{x:.1f}" y="{y:.1f}" font-family="{family}" font-size="{size}" '
             f'font-weight="{weight}" fill="{color}" text-anchor="{anchor}" '
             f'dominant-baseline="{"alphabetic" if vcap else "central"}"')
    if fit:
        attrs += f' textLength="{fit}" lengthAdjust="spacingAndGlyphs"'
    if style:
        attrs += f' style="{style}"'
    if ident:
        attrs += f' id="{ident}"'
    parts = []
    shift = 0.0
    for text, flags in segs:
        a = []
        if "r" in flags:
            a.append(f'fill="{RED_PRINT}"')
        if "k" in flags:
            a.append(f'fill="{BLACK_PRINT}"')
        if "w" in flags:
            a.append('fill="#ffffff"')
        if "i" in flags:
            a.append(f'font-family="{FONT_MATH}" font-style="italic"')
        if "c" in flags:
            a.append(f'font-family="{FONT_COND}"')
        if "b" in flags:
            a.append('font-weight="700"')
        if "a" in flags:
            tcol = RED_PRINT if "r" in flags else BLACK_PRINT if "k" in flags else color
            text = "→"
            a.append(f'style="stroke:{tcol};stroke-width:{size * 0.07:.1f};stroke-linejoin:round"')
        if "g" in flags:
            a.append(f'font-family="{FONT_GEO}"')
        if "s" in flags and "^" not in flags:
            a.append(f'font-size="{size * 0.72:.1f}"')
        want = -0.30 * size if "^" in flags else (0.25 * size if "_" in flags else 0.0)
        if "a" in flags:  # lift arrows to the middle of the cap height
            want = -0.10 * size
        if "u" in flags:  # small lift, e.g. the vinculum of a root sign
            want = -0.47 * size
        if "^" in flags or "_" in flags:
            a.append(f'font-size="{size * 0.70:.1f}"')
        if want != shift:
            a.append(f'dy="{want - shift:.1f}"')
            shift = want
        parts.append(f'<tspan {" ".join(a)}>{escape(text)}</tspan>')
    emit(f'<text {attrs}>{"".join(parts)}</text>')


def icon(kind, cx, cy, s, color):
    """Small vector glyphs drawn as paths, centred at cx,cy, scale s (~font size)."""
    k = s / 20.0
    if kind == "down" or kind == "up":
        sg = 1 if kind == "down" else -1
        d = (f"M{cx - 2*k:.1f},{cy - 9*k*sg:.1f} L{cx + 2*k:.1f},{cy - 9*k*sg:.1f} "
             f"L{cx + 2*k:.1f},{cy + 1*k*sg:.1f} L{cx + 7*k:.1f},{cy + 1*k*sg:.1f} "
             f"L{cx:.1f},{cy + 10*k*sg:.1f} L{cx - 7*k:.1f},{cy + 1*k*sg:.1f} "
             f"L{cx - 2*k:.1f},{cy + 1*k*sg:.1f} Z")
    elif kind in ("left", "right"):
        sg = 1 if kind == "right" else -1
        d = (f"M{cx - 12*k*sg:.1f},{cy - 8*k:.1f} L{cx + 12*k*sg:.1f},{cy:.1f} "
             f"L{cx - 12*k*sg:.1f},{cy + 8*k:.1f} Z")
    elif kind == "updown":
        d = (f"M{cx:.1f},{cy - 13*k:.1f} L{cx + 8*k:.1f},{cy - 3*k:.1f} L{cx + 2.3*k:.1f},{cy - 3*k:.1f} "
             f"L{cx + 2.3*k:.1f},{cy + 3*k:.1f} L{cx + 8*k:.1f},{cy + 3*k:.1f} L{cx:.1f},{cy + 13*k:.1f} "
             f"L{cx - 8*k:.1f},{cy + 3*k:.1f} L{cx - 2.3*k:.1f},{cy + 3*k:.1f} L{cx - 2.3*k:.1f},{cy - 3*k:.1f} "
             f"L{cx - 8*k:.1f},{cy - 3*k:.1f} Z")
    elif kind == "sqrt":
        emit(f'<path d="M{cx - 15*k:.1f},{cy + 1*k:.1f} L{cx - 11*k:.1f},{cy - 1*k:.1f} L{cx - 6*k:.1f},{cy + 10*k:.1f} '
             f'L{cx - 1*k:.1f},{cy - 10*k:.1f} H{cx + 16*k:.1f}" fill="none" stroke="{color}" '
             f'stroke-width="{3.6*k:.1f}" stroke-linejoin="miter"/>')
        return
    elif kind == "dot":
        emit(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{4.2*k:.1f}" fill="{color}"/>')
        return
    elif kind == "arrow":  # small solid right arrow (used inside text-like labels)
        d = (f"M{cx - 7*k:.1f},{cy - 1.6*k:.1f} H{cx + 1*k:.1f} V{cy - 5*k:.1f} L{cx + 7*k:.1f},{cy:.1f} "
             f"L{cx + 1*k:.1f},{cy + 5*k:.1f} V{cy + 1.6*k:.1f} H{cx - 7*k:.1f} Z")
    else:
        raise ValueError(kind)
    emit(f'<path d="{d}" fill="{color}"/>')


# ---------------------------------------------------------------- output
def svg_keys(svg):
    """The key rectangles of a generated SVG, {name: (x, y, w, h)}: every key is a group
    <g id="key-<name>" class="key" data-x=.. data-y=.. data-w=.. data-h=..> in canvas units."""
    import re
    return {m[0]: tuple(float(v) for v in m[1:])
            for m in re.findall(r'id="key-([^"]+)" class="key" data-x="([^"]+)" data-y="([^"]+)" '
                                r'data-w="([^"]+)" data-h="([^"]+)"', svg)}


def keymap(svg):
    """The key rectangles of a generated SVG for its layout JSON, {name: {x, y, w, h}}."""
    return {k: dict(zip("xywh", r)) for k, r in svg_keys(svg).items()}


def render_png(svg_path):
    """Render the SVG next to it as PNG with headless Chrome (render.sh), then optimise the PNG
    losslessly with oxipng."""
    import subprocess

    import oxipng
    png = Path(svg_path).with_suffix(".png")
    subprocess.run([str(HERE / "render.sh"), str(svg_path), str(png)], check=True)
    before = png.stat().st_size
    oxipng.optimize(png, level=4, strip=oxipng.StripChunks.safe())
    tag_srgb(png)
    print(f"{png.name}: {before // 1024} KB -> {png.stat().st_size // 1024} KB")


def tag_srgb(png):
    """Mark the PNG as sRGB (an sRGB chunk after IHDR), the colour space the faceplates are
    drawn in. Chrome writes the screenshot untagged, and macOS shows an untagged image in
    another colour space: on a wide-gamut display the blacks turn grey."""
    import struct
    import zlib
    data = Path(png).read_bytes()
    if b"sRGB" in data[:64] or b"iCCP" in data[:64]:
        return
    end = 8 + 8 + struct.unpack(">I", data[8:12])[0] + 4   # signature, IHDR
    body = b"sRGB" + b"\x00"                                # rendering intent: perceptual
    chunk = struct.pack(">I", 1) + body + struct.pack(">I", zlib.crc32(body))
    Path(png).write_bytes(data[:end] + chunk + data[end:])


def finish(svg_path, svg, layout, keep_svg=False, no_png=False):
    """Write the SVG and its layout (<name>-layout.json next to it), render the PNG and delete
    the SVG unless keep_svg (or no_png)."""
    import json
    svg_path = Path(svg_path)
    svg_path.write_text(svg)
    (svg_path.parent / f"{svg_path.stem}-layout.json").write_text(json.dumps(layout, indent=1))
    if no_png:
        return
    render_png(svg_path)
    if not keep_svg:
        svg_path.unlink()
