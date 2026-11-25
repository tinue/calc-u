# Calc-U-1600 faceplates (PC-1500 / PC-1500A / PC-1600 / PC-1600K)

`build.sh` regenerates, from the generators in this folder and the kit in
`tools/faceplate/`:

| Output | |
|---|---|
| `frontends/qt6/resources/faceplates/pc1500.png`, `pc1500A.png` | the PC-1500 and PC-1500A faceplates (4000 x 1760), LCD off |
| `frontends/qt6/resources/faceplates/pc1600.png` | the PC-1600 faceplate (4000 x 1777), LCD off |
| `faceplate/pc1600K.png` | the PC-1600K faceplate (4000 x 1777), LCD off; here until the product has the model |
| `app/KeyLayout/PC1500KeyLayout.cpp`, `PC1600KeyLayout.cpp` | the key rectangles (`keys.py`; the PC-1500A shares the PC-1500's, the PC-1600K the PC-1600's) |

The Qt and Web frontends both take the images from `frontends/qt6/resources/faceplates/`.

| File | |
|---|---|
| `build_svg_1500.py` | the PC-1500 / PC-1500A (`--model 1500|1500a`) |
| `build_svg_1600.py` | the PC-1600 / PC-1600K (`--model 1600|1600k`); shares keys, filters and legend placement with the 1500 |
| `keys.py` | writes the key tables from the faceplate SVGs |

Colours, key caps and printed details follow 600 dpi scans of a PC-1500, a
PC-1500A and a PC-1600 (not included); the PC-1600K's Japanese print follows
three photos of one (not included). The brushed aluminium keeps the PNGs
large (~2.7 MB).

A preview with a lit LCD:

```sh
uv run faceplate/build_svg_1500.py --text '10 PRINT "HELLO, WORLD!"' --cursor 24 --segments DE,G,RUN,BATT -o /tmp/preview.svg
uv run faceplate/build_svg_1600.py --text '10 PRINT "Hello, World!"' --text RUN --cursor 3,13 --segments RUN,BATT -o /tmp/preview.svg
```

Options: `--model 1500|1500a` (`build_svg_1500.py`), `--model 1600|1600k` (`build_svg_1600.py`), `--text`, `--cursor`,
`--segments`, `--keep-svg`, `--no-png`, `-o` (required; the PNG and
`<name>-layout.json` go next to it).

Each generator's header lists the layers of its SVG and what the key rectangles are.
