# tools/

Tools of the source repository that belong to no one product. Each
product's own tools are in `products/<product>/tools/`.

## help: the help tool

`tools/help` checks a product's help and builds its help asset
(`docs/formats/help.md`). Python 3, standard library only; no install.

```sh
tools/help check calc-u-1600                 # the checks, no asset
tools/help build calc-u-1600                 # the checks, then build/help/Calc-U-1600-help.zip
tools/help check calc-u-59                   # Calc-U-59's help
tools/help build 59 --version 3.0.0 --out /tmp/help
```

The product is named by its name or short form, any case (`Calc-U-1600`,
`1600`, `Calc-U-59`, `59`). Each product's CTest entries
`<Product>HelpCheck` and `<Product>HelpBuild` run them in Frequent. The help's sources:

- `library/help/`: the shared parts, one file per chapter that has one;
  `help.yaml` lists the ten chapters, their parts and their shared
  files, the tool's one list of them;
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
names a product word or has a picture (Mermaid is fine); no help file
names a source path or a milestone (`M-<n>`); every settings key is
described in chapter 8; `chapter:<n>` links (a shared part's way to the product's part
of chapter n) resolve. The words the glossary lists under "Avoid" are
warnings (`--quiet-warnings` counts them only).

The asset holds `toc.json` (the product, the version, the chapters and
their files, each marked shared or not), `shared/`, `product/` with its
pictures and downloads; links are rewritten to their places in the zip,
and two builds of one tree give the same bytes.

Tests: `tools/helptool/tests/` (`python3 -m unittest discover -s
helptool/tests -t .` in `tools/`; CTest `LibraryHelpTool`), on the fixture
help tree `tools/helptool/tests/fixtures/good/`.

## faceplate/: the kit of the faceplate generators

The fonts, the SVG helpers and the rendering the products' faceplate
generators share (`tools/faceplate/README.md`). Each product's
`products/<product>/faceplate/build.sh` writes its faceplate images and key
table; the outputs are committed.

## run_tests.sh: a product's Frequent tests

```sh
tools/run_tests.sh calc-u-59           # every Frequent CTest entry of the product
tools/run_tests.sh calc-u-1600 ce158   # the TESTs whose name contains ce158, in each runner program
```

Builds the product's runner programs (and `calcu`) in the repository's
`build/tests` tree, configured the first time as Debug without the Qt apps
(the cores and the test programs at `-O1`), and runs them from the product
directory. The product is `calc-u-59` or `59`, `calc-u-1600` or `1600`.

## fetch_roms_common.sh: the ROM download both products share

`products/<product>/roms/fetch_roms.sh` fetches a product's ROM files: it
is the product's fetch list (each file's name, md5 and source, and the
environment variable that overrides a source repository's base URL) and
sources `tools/fetch_roms_common.sh`, which downloads, joins the parts of
a file kept upstream per bank, checks the md5 and skips a file that is
already there.

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

## check_toolchain_versions.sh: one version of each pinned toolchain

```sh
tools/check_toolchain_versions.sh   # exit 1 and the list of places when they differ
```

Checks that the pinned toolchains have one version everywhere:
Playwright in the three `package-lock.json` files (Calc-U-59's and
Calc-U-1600's Web frontends, `library/web/`), the Playwright container of
both product workflows' `web` jobs and the local CI's web image
(`ci/linux/Containerfile`), since the container's browsers belong to one
Playwright release; Emscripten in both `web` jobs' setup-emsdk, the local
CI's `EMSDK_VERSION` and the release `libsharpdx-wasm.a` was built with
(its README, `docs/Building.md`); Node's major version in both `web` jobs'
setup-node and the local CI's `NODE_VERSION`. Reads files only. CTest
`LibraryToolchainVersions` (Frequent; not on Windows).

## check_chrome_colours.sh: no colour literal in a frontend

```sh
tools/check_chrome_colours.sh <product folder | library folder>
```

The chrome's colours live in the product's `app/` (`TIChrome`,
`SharpChrome`) and the library's `shell/Chrome`, and every GUI reads them:
the script fails on a colour literal in a frontend's code or the library's
component sets, but where the folder's allow-list says why. CTests
`LibraryChromeColours`, `CalcU59ChromeColours`, `CalcU1600ChromeColours`
(Frequent).

## build-id.cmake: the build ID, the package and the build time

```sh
cmake -DPRODUCT=<product directory> -P tools/build-id.cmake   # prints the ID
```

The ID that About and the Settings' version line show after the version
(`<hash>`, `pre-<hash>`, `-dirty`; docs/Process.md, "What a build shows").
The builds call it with `HEADER` (`APP_BUILD_ID`, `APP_PACKAGE`: DMG,
AppImage, Windows installer, else self-built) and `TIME_SOURCE` (the
build's time in UTC, `calcuAppBuildTime()`), which About's Support
Information shows.
