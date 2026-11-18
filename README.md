# Calc-U

Calc-U is a family of emulators for vintage calculators and pocket
computers. Each product runs the original ROMs of its machines and comes
with a debugger, presets, and the media and peripherals of the real
hardware. Both products are built from this repository on one shared
library, and each is released on its own.

| | **Calc-U-59** | **Calc-U-1600** |
|---|---|---|
| Machines | Texas Instruments TI-59, TI-58, TI-58C | Sharp PC-1500, PC-1500A, PC-1600 |
| Peripherals and media | PC-100C printer, magnetic cards, Solid State ROM modules, the TI-58C's constant memory | CE-150 and CE-1600P printer/plotters, CE-158 serial interface, memory modules, CE-1600F floppy disks, cassettes, a host-folder drive |
| Platforms | macOS, iPhone and iPad (Swift), Windows and Linux (Qt6), the browser (WebAssembly) | macOS, Windows and Linux (Qt6), the browser (WebAssembly) |
| Debugger | Calculator and CPU level, traces, ASM overlays | CPU level for the LH5801/LH5803 and Z-80, DAP for VS Code |
| User guide | <https://calc-u.org/59/> | <https://calc-u.org/1600/> (being built) |
| Source | [`products/calc-u-59/`](products/calc-u-59/) | [`products/calc-u-1600/`](products/calc-u-1600/) |
| Releases | tags `calc-u-59-v…` | tags `calc-u-1600-v…` |

## Getting a product

Releases are on this repository's
[Releases page](https://github.com/tinue/calc-u/releases): the macOS DMG,
Windows installers (x64, ARM64), Linux AppImages (x86_64, aarch64), the
Web frontend as a zip, and the user guide as a help zip. Calc-U-59 for
iPhone and iPad is on the App Store. The first releases from this
repository, Calc-U-59 3.0.0 and Calc-U-1600 0.9.0, are in preparation;
their pre-releases are already published.

The ROM images are not part of this repository. Each product fetches
them from their own repositories with `roms/fetch_roms.sh`.

## Repository layout

```
library/<area>/        the shared library, calcu_library (sessions, presets,
                       media, debugger, harness, settings, Qt6 and Web shells, …)
library/help/          the user-guide chapters both products share
products/calc-u-59/    Calc-U-59: core/ app/ frontends/ help/ examples/ tests/ reference/
products/calc-u-1600/  Calc-U-1600: core/ app/ frontends/ help/ examples/ tests/ docs/
docs/                  formal specifications and project documents (below)
tools/  ci/            the help tool, the local Linux CI
```

## Building

One root CMake project builds both products, their tests and the Qt6
apps (C++20; Qt 6 for the desktop apps, Emscripten for the Web
frontends, Xcode for the Swift app):

```sh
products/calc-u-59/roms/fetch_roms.sh
products/calc-u-1600/roms/fetch_roms.sh
cmake -S . -B build/tests -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCALCU_FRONTEND_QT=OFF
cmake --build build/tests
ctest --test-dir build/tests -j8
```

Options: `CALCU_PRODUCT_59`, `CALCU_PRODUCT_1600`, `CALCU_FRONTEND_QT`,
`CALCU_TESTS`, `CALCU_TESTS_LONGER`, `CALCU_TESTS_CHECKPOINT`. Each
product's developer documents say how to build its frontends:
[Calc-U-59](products/calc-u-59/README.md),
[Calc-U-1600](products/calc-u-1600/docs/developer/Building.md).

## Further information

- **User guides:** Markdown in [`library/help/`](library/help/) and each
  product's `help/`, rendered at <https://calc-u.org>.
- **File formats and interfaces:** [`docs/formats/`](docs/formats/README.md)
  — presets, media, modules, cue cards, the bundle file, opening files,
  the help asset, the Web embedding interface, the harness `calcu` and
  the session protocol.
- **Product references:** [Calc-U-59](products/calc-u-59/reference/)
  (architecture, debug API, harness commands, Web client),
  [Calc-U-1600](products/calc-u-1600/docs/developer/README.md)
  (app layer, debugger and DAP, harness commands, core limitations).
- **Terms:** [`docs/GLOSSARY.md`](docs/GLOSSARY.md).
- **Tests:** [`docs/test-groups.md`](docs/test-groups.md) — the Frequent,
  Longer and Checkpoint groups and the local Linux CI.
- **Development and releases:** [`docs/Process.md`](docs/Process.md);
  until the first releases are out, [`docs/phase-f.md`](docs/phase-f.md).
- **Changes:** each product's `CHANGELOG.md`.
- **History:** the requirements, plans and design decisions that brought
  both products into this repository are on the branch
  [`history`](https://github.com/tinue/calc-u/tree/history); the full
  snapshot at that point is the tag `history-phases-0-6`. The products'
  earlier history is in their own repositories,
  [tinue/Calc-U-59](https://github.com/tinue/Calc-U-59) and
  [tinue/Calc-U-1600](https://github.com/tinue/Calc-U-1600).

## License

GNU General Public License, version 3: [`LICENSE`](LICENSE). Each product
lists its third-party notices in its `THIRD-PARTY-NOTICES.md`.
