# tools/faceplate: the faceplate kit

What the products' faceplate generators (`products/<product>/faceplate/`)
share:

| File | |
|---|---|
| `svgkit.py` | the embedded fonts, the SVG output buffer and layers, rich text, small vector icons, `svg_keys` (the key rectangles of a generated SVG), `render_png` and `finish` (SVG → PNG) |
| `render.sh` | `render.sh in.svg out.png [scale]`: renders an SVG with headless Chrome (`CHROME` names another browser) |
| `fonts/` | TeX Gyre Heros, Heros Cn, Adventor, Termes (GUST Font License) |

Every key of a generated SVG is a group `<g id="key-<name>" class="key"
data-x=.. data-y=.. data-w=.. data-h=..>` in the faceplate's pixels; a
product's `faceplate/keys.py` writes its C++ key table from them.

A product's `faceplate/build.sh` renders its faceplates, copies the PNGs to
the frontends' resources and rewrites the key table. It is run by hand after
a generator changed, and its outputs are committed: no build renders. It needs
`uv` (the scripts declare their Python packages: fontTools, pyoxipng) and
Google Chrome.
