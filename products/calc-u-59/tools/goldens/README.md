# Golden data for the app/ ports

Logic that moved from the Swift app into `app/` (C++) is checked against
the Swift original it replaced. Each `*_main.swift` here is a small driver
that is compiled together with that original Swift file — read from the git
commit that still had it — and prints its results. `gen.sh` writes them to
`tests/golden/<name>.txt`, and the C++ tests in `tests/` compare the port
against them.

```sh
tools/goldens/gen.sh            # all goldens
tools/goldens/gen.sh keynames   # one
```

The goldens are committed. Regenerate only when a driver changes; the
reference is the Swift code at the pinned commit, not the current tree.

| Golden | Swift original (commit) | C++ port | Test |
|---|---|---|---|
| `keynames` | `frontends/swift/App/TI59KeyNames.swift` (66537d3) | `app/KeyNames.cpp` | `tests/data_tables_tests.cpp` |
| `formats` | `frontends/swift/App/StateFileLoader.swift`, `CueCardContent.swift`, `RegisterLineHelpers.swift`, `MachineModel.swift` (66537d3) | `CueCard.cpp`, `Bcd.cpp` (the state-file parser and its `STATE` records went in M-54, the card files and their `CARD` records in M-55) | `tests/file_formats_tests.cpp` |

The `formats` driver dumps the register-line and BCD records (the `CARD`
records of 2.3.0's card files went in M-55, C59-7 TR). The `STATE` records of
2.3.0's state files went in M-54 (C59-2 §2): presets of format 2 are the
library's, checked by `calcu_preset_check.CalcU59Presets` and the reference
runs.
