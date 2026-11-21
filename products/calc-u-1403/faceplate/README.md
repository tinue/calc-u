# Calc-U-1403 faceplates (PC-1401 / PC-1402 / PC-1403H / PC-1421)

`pc1401.png`, `pc1402.png`, `pc1403h.png`, `pc1421.png`: the faceplates
(2576 x 1150), LCD off, written by `build.sh` from `build_svg.py` and the kit
in `tools/faceplate/`. The four models share the panel; their differences are
listed in `MODELS`.

A preview with a lit LCD:

```sh
uv run faceplate/build_svg.py --model 1403h --text '10PRINT"Hello, World!"' --cursor 22 --segments model -o /tmp/preview.svg
```

Options: `--model 1401|1402|1403h|1421`, `--text`, `--cursor`, `--segments`
(`model`: the model's own), `--keep-svg`, `--no-png`, `-o` (required; the PNG
and `<name>-layout.json` go next to it).
