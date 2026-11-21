# Calc-U-59 faceplate and cue cards

`build.sh` regenerates, from the generators in this folder and the kit in
`tools/faceplate/`:

| Output | |
|---|---|
| `frontends/swift/App/Assets.xcassets/ti59_base`, `ti58_base` | the faceplates (2064 x 4075), LED off, no card; the TI-58 / TI-58C one differs only in the missing `Write` legend |
| `frontends/swift/App/Assets.xcassets/CueCard`, `MagnetCard`, `SolidStateCard` | the cue card templates (2064 x 440) |
| `app/KeyboardLayout.hpp` `kKeyRects` | the key rectangles: the caps of the faceplate (`keys.py`) |

The Swift, Qt and Web frontends all take the images from `Assets.xcassets`.

| File | |
|---|---|
| `build_svg_ti59.py` | the faceplate (`--model 59|58`) |
| `build_cards.py` | the three cue card templates |
| `ti59-ink.json` | the sizes and colours of every legend and cap label, measured on a reference rendering |
| `keys.py` | writes `kKeyRects` from the faceplate SVGs |

A preview with lit LEDs or a fitted card:

```sh
uv run faceplate/build_svg_ti59.py --led 3.141592654 --card frontends/swift/App/Assets.xcassets/MagnetCard.imageset/MagnetCard.png -o /tmp/preview.svg
```

Options of `build_svg_ti59.py`: `--model 59|58`, `--led TEXT`, `--card PNG`,
`--legend target|warm|rich|card|satin|metal` (legend ink, default warm),
`--keep-svg`, `--no-png`, `-o` (required; the PNG and `<name>-layout.json`
go next to it). `build_cards.py`: `--keep-svg`, `--no-png`, `-o DIR`.

## The faceplate

- **Layout**: the emulator's, not the original calculator's: the canvas of
  `app/KeyboardLayout.hpp` (`kDisplayRect`, `kCardRect`), the LED cells of
  `app/LedGeometry`.
- **Look**: flat keys (a cap in a narrow black gap), lit from the top: a highlight along
  each cap's top edge that wraps around its top corners, and a shadow below each
  well; flat black panel. Type and
  signs after a TI-58C: Helvetica (TeX Gyre Heros), bold italic serif (TeX
  Gyre Termes) for x and y. Warm gold legends; the lettering on the brown
  caps is the cream of the number keys.

The layers of the SVG are listed in the header of `build_svg_ti59.py`.

## The cue cards

Straight rules of one width (8), a regular column pitch, one corner radius per
card. The cells line up with the app's text layout (`app/CueCard.cpp`
`cueCardLayout`); the MagnetCard arrows are the app's own points
(`cardCornerGeometry`), as the app redraws them on a lit corner. The layers are
listed in the header of `build_cards.py`.
