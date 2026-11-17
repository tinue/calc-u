# tools/

Tools of the source repository that belong to no one product. Each
product's own tools are in `products/<product>/tools/`.

## help: the help tool (LIB-16)

`tools/help` checks a product's help and builds its help asset
(`features/LIB-16-help-content/`, ADR 0036). Python 3, standard library
only; no install.

```sh
tools/help check calc-u-1600                 # the checks, no asset
tools/help build calc-u-1600                 # the checks, then build/help/Calc-U-1600-help.zip
tools/help check calc-u-59                   # Calc-U-59's help (M-60)
tools/help build 59 --version 3.0.0 --out /tmp/help
```

The product is named by its name or short form, any case (`Calc-U-1600`,
`1600`, `Calc-U-59`, `59`). Each product's CTest entries
`<Product>HelpCheck` and `<Product>HelpBuild` run them in Frequent. The help's sources:

- `library/help/`: the shared parts, one file per chapter that has one;
  `help.yaml` lists the ten chapters (LIB-16 AC-1), their parts, their
  shared files and the specs each shared part follows (AC-3), the tool's
  one list of them;
  `anchor_cases.txt` the headings and anchors by GitHub's rule.
- `products/<product>/help/`: the product parts; `help.yaml` lists them per
  chapter, with the product's name, its version source (`version-from:`,
  read as ReleaseIt reads it: any of its presets `xcode`, `cmake`,
  `cargo`, `regex`) and `names:`, the words no shared part may
  use; `images/` the pictures; `downloads/` files the text offers;
  `settings-keys.txt` the settings keys the product declares (written by
  a product test).

A failure names the file and the line; the exit status is 0 without
failures, 1 with, 2 for a usage error. The checks: the ten chapters with
their parts; every relative link, anchor and picture leads somewhere;
every help file is in a manifest and every picture used; no shared part
names a product word or has a picture (Mermaid is fine), and each starts
with `<!-- follows: <specs> -->`; every settings key is described in
chapter 8; `chapter:<n>` links (a shared part's way to the product's part
of chapter n) resolve. The words the glossary lists under "Avoid" are
warnings (`--quiet-warnings` counts them only).

The asset holds `toc.json` (the product, the version, the chapters and
their files, each marked shared or not), `shared/`, `product/` with its
pictures and downloads; links are rewritten to their places in the zip,
and two builds of one tree give the same bytes.

Tests: `tools/helptool/tests/` (`python3 -m unittest discover -s
helptool/tests -t .` in `tools/`; CTest `LibraryHelpTool`), on the fixture
help tree `tools/helptool/tests/fixtures/good/`.

## run_qt.sh: a product's Qt6 app, built and started

```sh
tools/run_qt.sh 59 [file.ti59]   # build build/app-59 (the other product off) and start the app
tools/run_qt.sh 1600 --build     # build only
tools/run_qt.sh 1600 --app-dir   # print the directory holding the built app
```

The local development build (Release, Ninja; configured the first time
only). On macOS the `.app` is started through `open`, else the binary;
arguments after the product go to the app. Calc-U-1600's
`tools/make_screenshots.sh` builds through it.
