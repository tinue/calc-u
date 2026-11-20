# Golden data for the app/ ports

Logic that moved from the Swift app into `app/` (C++) is checked against
the Swift original it replaced. Each `*_main.swift` here is a small driver
that is compiled together with that original Swift file — read from the git
commit that still had it — and prints its results. `gen.sh` writes them to
`tests/golden/<name>.txt`, and the C++ tests in `tests/` compare the port
against them.

The pinned commit 66537d3 is one of the former Calc-U-59 repository, whose
history is not part of this repository: `gen.sh` runs only in a clone of
that repository with this folder copied in. Here the committed goldens are
the reference.

```sh
tools/goldens/gen.sh            # all goldens
tools/goldens/gen.sh keynames   # one
```

The goldens are committed. Regenerate only when a driver changes; the
reference is the Swift code at the pinned commit, not the current tree.

| Golden | Swift original (commit) | C++ port | Test |
|---|---|---|---|
| `keynames` | `App/TI59KeyNames.swift` (66537d3) | `app/KeyNames.cpp` | `tests/data_tables_tests.cpp` |
| `formats` | `App/StateFileLoader.swift`, `CueCardContent.swift`, `RegisterLineHelpers.swift`, `MachineModel.swift` (66537d3) | `CueCard.cpp`, `Bcd.cpp` (without the state-file parser and its `STATE` records, and without the card files and their `CARD` records: neither file is read any more) | `tests/file_formats_tests.cpp` |

The `formats` driver dumps the register-line and BCD records. It dumps no
`CARD` records of 2.3.0's card files (magnetic cards are card format 3 now)
and no `STATE` records of 2.3.0's state files: presets of format 2 are the
library's, checked by `calcu_preset_check.CalcU59Presets` and the reference
runs.
