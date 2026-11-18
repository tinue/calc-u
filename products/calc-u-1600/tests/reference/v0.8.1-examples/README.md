# Reference runs of the 32 Calc-U-1600 examples at `v0.8.1`

Recorded before any file was upgraded and before any core code was
touched. They are compared with the upgraded presets at the idle machine
after the load.

Made with the CLIs of Calc-U-1600 `v0.8.1` (tree `1c36e0f`, the pin
`b4ea8c8` plus a docs-only delta), built by `tools/build_cli.sh` and
`tools/build_pc1600_cli.sh` from `git archive v0.8.1`, ROMs from
`tools/fetch_roms.sh`, by `run.sh <v0.8.1 tree> <out dir>`: each example in
a fresh copy of `examples/`, with `--lcd-text -`, `--inspect pointers`,
`basic` (PC-1500) or `programs` and `--dump-basic` (PC-1600),
`dump-basic` and `dump-ml`. The path of the copy is written `<copy>`.

Per example `<folder>_<file>`:
- `.out`: what the CLI prints after the load: the display as text and the
  status symbols, the inspector views (BASIC pointers, program area,
  machine-code area as hex), and for the CE-150 its pen events and point
  count (nothing of the CE-1600P's paper);
- `.log`: the `[preset]` log of the load, item by item, with the display
  and the processor's position after each;
- `.meta`: exit code, seconds, the files the run changed in the copy of
  `examples/`, the media saved by name.

- `.paper` (the 14 examples with a plotter): the paper as plot points,
  one per line (`x y color newStroke`) after a header per plotter. Added
  from the code as imported (the 0.8.1 code in
  `products/calc-u-1600`), before any core change, with the imported CLIs
  and `CALCU_PAPER_OUT=<file>` (added to them for this,
  `tools/CliCommon.hpp`), each example again in a fresh copy of
  `examples/`. They are turned into `paper` replies once.
- `.<plotter>.paper-marks` (`CE-150` or `CE-1600P`, one per plotter block
  of each `.paper` file): the plot as the reply of the harness command
  `paper marks`, written once by
  `tests/paper_replies_tests.cpp` with `CALCU1600_PAPER_REPLIES=1`: a pen
  stroke from each point with `newStroke` 1, in the pen's colour, its
  positions moved by the plotter's left margin (66.9474 or 101.0526 units
  of 190/1920 mm); the reach is the range of the points' y (the files hold
  no feeds). The Frequent case of that file checks that
  they equal what the converter gives; `CalcU1600ExampleReferences`
  compares new runs with them.
