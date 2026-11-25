# tools/faceplate: the faceplate kit

What the products' faceplate generators (`products/<product>/faceplate/`)
share:

| File | |
|---|---|
| `svgkit.py` | the embedded fonts (`font_faces`, `face`) and their metrics (`font`, `advance`, `glyph_runs`), the SVG output buffer (`svg_open`, `emit`, `layer`, `end`), rich text and small vector icons, the key rectangles of a generated SVG (`svg_keys`, `keymap`), the rendering (`render_png`, `finish`: SVG → PNG and layout JSON) |
| `lcdfont.py` | the 5 x 7 LCD font of the Sharp faceplates' previews |
| `render.sh` | `render.sh in.svg out.png [scale]`: renders an SVG with headless Chrome (`CHROME` names another browser) |
| `fonts/` | TeX Gyre Heros, Heros Cn, Adventor, Termes (GUST Font License); Noto Sans JP (SIL Open Font License), cut to the Japanese characters of the faceplates (`svgkit.JAPANESE`) |

Every key of a generated SVG is a group `<g id="key-<name>" class="key"
data-x=.. data-y=.. data-w=.. data-h=..>` in the faceplate's pixels; a
product's `faceplate/keys.py` writes its C++ key table from them.

A product's `faceplate/build.sh` renders its faceplates, copies the PNGs to
the frontends' resources and rewrites the key table. It is run by hand after
a generator changed, and its outputs are committed: no build renders. It needs
`uv` (the scripts declare their Python packages: fontTools, pyoxipng) and
Google Chrome.

Every generator starts with the same header: what it draws, the coordinate
system and where it was measured, the sources of the look, its layers (bottom
to top, what `data-x/y/w/h` of a key is) and its usage.
