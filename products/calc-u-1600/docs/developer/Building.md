# Building Calc-U-1600

This is the developer build guide: prerequisites, getting the ROMs, and
building the Qt6 desktop app on macOS, Linux, and Windows. For how to
*use* the emulator once it's built, see [User-Guide.md](../User-Guide.md).

Everything here is derived from the project's own recipes — the
repository's root [`CMakeLists.txt`](../../../../CMakeLists.txt) (ADR 0003),
this product's [`CMakeLists.txt`](../../CMakeLists.txt) and
[`frontends/qt6/CMakeLists.txt`](../../frontends/qt6/CMakeLists.txt), and
the CI workflow
[`.github/workflows/calc-u-1600.yml`](../../../../.github/workflows/calc-u-1600.yml)
at the repository root — which is also where to look first if a step below
stops matching reality. The macOS steps are exercised on an actual Mac as
part of writing this doc; the Linux and Windows steps are transcribed from
CI and not independently verified on a fresh machine — if something's off,
please open an issue or PR.

All C++ builds go through the root CMake project, configured from the
repository root (`-S ../..` from this directory). Its options choose what
is built: `CALCU_PRODUCT_59`, `CALCU_PRODUCT_1600` and `CALCU_FRONTEND_QT`
(all ON by default). This directory does not configure on its own, except
under Emscripten until the Web frontends move to the root (M-12 of the
migration plan). The commands below run from this directory and put their
trees under the repository's `build/`.

## Prerequisites

- **CMake** 3.21 or newer.
- **Qt 6.3 or newer**, with the `Widgets`, `Multimedia` (the buzzer's
  audio output) and `Network` (the debugger's server) components. Older Qt6 (e.g.
  Ubuntu 22.04's packaged 6.2.4) doesn't provide
  `qt_standard_project_setup()` and won't configure.
- A **C++20** compiler (plain portable C++; the long machine operations
  are C++20 coroutines, core/Job.hpp): Xcode clang, GCC 11+, MSVC 2019
  16.8+.
- On macOS only: Xcode command-line tools (for the Objective-C++
  compiler — three files need it: `MacClipboardImage.mm` for the
  plotter-paper clipboard export, `MacAppSupport.mm` and
  `screenshots/MacShotSupport.mm`).

### `libsharpdx`

Every program loader (Load BASIC Program…, Load Machine Code…, presets,
the debugger's Build & Load) reads files through a vendored Rust static
library, `core/Basic/vendor/sharpdx/`: it tells what a file is and
tokenizes BASIC listings.

- On **macOS** it's committed directly to the repo — nothing to do.
- On **Linux/Windows** fetch it before configuring with
  `tools/fetch_sharpdx.sh` (see below) from a `SharpDataExchange`
  GitHub release.

It is required: CMake stops with an error if the library is missing. You
don't need a Rust toolchain; only a prebuilt `.a`/`.lib` is needed, and CI
fetches or commits it for you.

## 1. Get the source

```sh
git clone <this repository's URL>
cd calc-u/products/calc-u-1600
```

## 2. Get the ROMs

The emulator needs real Sharp ROM images, which aren't committed to the
repo (see [`roms/README.md`](../../roms/README.md) for provenance/licensing).
Fetch and verify them before configuring:

```sh
roms/fetch_roms.sh
```

Skipping this step doesn't fail the build — CMake just globs an empty
`roms/` directory — but the resulting binary ships with no firmware and
won't boot any machine.

On Linux/Windows, fetch `libsharpdx` too (see above):

```sh
tools/fetch_sharpdx.sh
```

## 3. Configure & build

### macOS

```sh
brew install qt cmake ninja
tools/build_app.sh
```

`tools/build_app.sh` configures the root project once
(`cmake -S ../.. -B ../../build/app-1600 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCALCU_PRODUCT_59=OFF`;
Homebrew's Qt is found automatically) and builds it. The app is
`../../build/app-1600/products/calc-u-1600/frontends/qt6/Calc-U-1600.app`,
a real macOS bundle (`tools/build_app.sh app_dir` prints its directory;
`./build_and_run.sh` builds and starts it). To run it standalone outside
this checkout (e.g. hand it to another Mac), also run `macdeployqt` to
embed the Qt frameworks:

```sh
$(brew --prefix qt)/bin/macdeployqt "$(tools/build_app.sh app_dir)/Calc-U-1600.app"
```

Without that step the app still runs from this build directory (Qt is
found via the Homebrew prefix), just not as a portable, relocatable
bundle. The signing/notarizing/DMG steps CI does after this are
release-packaging concerns — see the `mac-aarch64` job in the workflow.

### Linux (best-effort — CI-derived, not verified on a fresh machine)

```sh
sudo apt-get install -y \
  build-essential cmake ninja-build patchelf zlib1g-dev \
  qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev qt6-wayland libgl1-mesa-dev \
  qt6-gtk-platformtheme qt6ct
tools/build_app.sh
```

This produces a plain `Calc-U-1600` executable plus a `resources/` folder
(bundled `.card.yaml` definitions and ROMs) that must stay next to it, in
`../../build/app-1600/products/calc-u-1600/frontends/qt6/`. That's enough
to run on the machine that built it. To package a standalone `.AppImage`
that runs on other Linux machines, see the `linux-x86_64`/`linux-aarch64`
jobs in the workflow for the full `linuxdeploy`/`linuxdeploy-plugin-qt`
recipe (it's involved enough — Wayland plugin handling included — that
it's not worth hand-transcribing here; run the workflow or copy its steps
directly).

### Windows (best-effort — CI-derived, not verified on a fresh machine)

```powershell
# Install Qt 6.8.x (MSVC, win64_msvc2022_64, with the qtmultimedia module)
# and Ninja by whatever means you prefer (CI uses the aqtinstall-based
# jurplel/install-qt-action).
cmake -S ..\.. -B ..\..\build\app-1600 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCALCU_PRODUCT_59=OFF
cmake --build ..\..\build\app-1600
```

Run this from a shell with the MSVC toolchain on `PATH` (a Visual Studio
"Developer Command Prompt", or `vcvarsall.bat`/`ilammy/msvc-dev-cmd`'s
equivalent). This produces `Calc-U-1600.exe` plus a `resources\` folder
that must stay next to it, in
`..\..\build\app-1600\products\calc-u-1600\frontends\qt6\`. To stage a
standalone-runnable copy (Qt DLLs + MSVC runtime bundled in) and package
it as an installer, run `windeployqt --release --compiler-runtime` on the
`.exe` and package the staged folder with Inno Setup using
[`frontends/qt6/resources/windows/calc-u-1600.iss`](../../frontends/qt6/resources/windows/calc-u-1600.iss)
— see the `windows-x86_64` job in the workflow for the exact staging
layout. The packaging jobs configure the repository root into
`build/release` with `-DCALCU_PRODUCT_59=OFF -DCALCU_TESTS=OFF` and build
only `CalcU1600Qt`; the `.iss` file includes its generated `Version.iss`
from that tree (`build/release/products/calc-u-1600/frontends/qt6/generated/`).
`CALCU_TESTS=OFF` leaves out `CoreTests` and with it the zlib the Windows
runners don't have. The installer is currently unsigned.

CI also builds a native ARM64 version (`windows-arm64` job) on a
`windows-11-arm` runner, the same way as x86_64 but with Qt's
`win64_msvc2022_arm64` kit, `ilammy/msvc-dev-cmd`'s `arm64` toolchain and
the `windows-aarch64` sharpdx release. It passes `/DAPP_ARCH=arm64` to
`ISCC.exe`, so the `.iss` above produces an ARM64-only
`Calc-U-1600-Setup-arm64.exe` instead. To build it yourself on a Windows
ARM64 machine, run the steps above from an ARM64 developer prompt
(`vcvarsall.bat arm64`) with the ARM64 Qt kit.

### CLion / IDE integration

Open the repository root in CLion and it configures the root project
with every product: the Qt6 apps, the headless CLI harnesses and probes,
and the test suites as CTest targets. It picks up the shared run/debug
configurations committed in the repository's `.run/`:

| Run configuration | CMake target | Notes |
|---|---|---|
| `Calc-U-1600` | `CalcU1600Qt` | The Qt6 GUI app |
| `CoreTests` | `CoreTests` | Headless test suite, also runnable via CTest |
| `pc1500_cli`, `pc1600_cli` | same | Headless CLI harnesses |
| `pc1600_plotter_probe`, `pc1600_power_probe`, `pc1600_uart_probe` | same | Hardware diagnostic probes |

All of them run and debug interactively (breakpoints, variable
inspection, stepping) — which is the main thing this build adds over
`tools/build_*.sh` and `tools/run_tests.sh`.

Run `roms/fetch_roms.sh` (step 2 above) first: `CoreTests` and the
`pc1500_cli`/probe configurations pass paths under `roms/`, and ROMs
aren't committed.

`core/` and `app/` come from [`cmake/CalcU.cmake`](../../cmake/CalcU.cmake),
which globs the sources into two static libraries: `calcu_core` and
`calcu_shared` (the frontend-neutral layer above Core); `calcu_add_core()`
makes a third, `calcu_core_probe`, with `PC1600_POWER_PROBE` for the two
probes that need it. A new `.cpp` under either directory needs no list
entry; re-run CMake (or let Ninja's glob check do it). No script lists
sources: `tools/build_*.sh` build the CMake targets
(`tools/build_tool.sh`, in the repository's `build/tools` tree) and copy
them into `headless/`.

## Running the tests

```sh
tools/run_tests.sh
```

Builds and runs the headless `core/` + `app/` test suite (no Qt, no
GUI): it configures the root project with `-DCALCU_FRONTEND_QT=OFF` into
the repository's `build/tests` tree (Debug, so Core and the tests compile
at `-O1`) and runs `CoreTests` from this directory, since the tests load
`roms/` via relative paths. An argument is passed on to `CoreTests` as a
filter: `tools/run_tests.sh ce158` runs the tests whose name contains
`ce158`. Needs CMake, Ninja and zlib (`zlib1g-dev` on Ubuntu; macOS has it).

The tests use the repository's one C++ test runner,
`library/testing/TestHarness.hpp` (ADR 0038): each file registers its
tests with `TEST(name)` and checks with `CHECK` / `CHECK_EQ`; a test that
needs the ROMs starts with `SKIP_UNLESS(...)`, which reports it as skipped
when they are missing. The runner's `main()` (target `calcu_test_runner`)
runs every registered test; there is no list to keep.

CI's `core-tests` job runs the same suite through CTest (as
`RelWithDebInfo`, with Calc-U-1600 only):

```sh
cmake -S ../.. -B ../../build/tests -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCALCU_PRODUCT_59=OFF -DCALCU_FRONTEND_QT=OFF
cmake --build ../../build/tests
ctest --test-dir ../../build/tests --output-on-failure
```

The tests are always built optimized: `CoreTests` and `calcu_core` compile
at `-O1` in a Debug build (which is what `tools/run_tests.sh` uses) — the suite runs about three times
faster than at `-O0`. To step through a failing test, build a separate
full-debug tree with `CALCU_TESTS_DEBUG=ON` (Core and the tests at `-O0`) (in CLion: add the option to a
CMake profile's options):

```sh
cmake -S ../.. -B ../../build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCALCU_FRONTEND_QT=OFF -DCALCU_TESTS_DEBUG=ON
cmake --build ../../build/debug --target CoreTests
../../build/debug/products/calc-u-1600/CoreTests   # from this directory
```

### Test groups

Every CTest entry carries a group label (ADR 0031): `frequent` (on every
change, ≤ 60 s: `CoreTests`, `CalcU1600CheckPresets`), `longer` (before
merging into `main`, ≤ 10 min: Playwright, the WebAssembly check, the
screenshot scenarios run as checks into the build tree, `tools/dap_smoke.py`)
and `checkpoint` (before a release: the User Guide screenshots, the
ten-minute pacing runs on the real clock). A tree has only the Frequent
group unless it is configured with `-DCALCU_TESTS_LONGER=ON` or
`-DCALCU_TESTS_CHECKPOINT=ON`; the screenshot and DAP entries also need
the Qt app in the tree, Playwright a built `frontends/web/dist/`:

```sh
cmake -S ../.. -B ../../build/groups -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCALCU_TESTS_LONGER=ON -DCALCU_TESTS_CHECKPOINT=ON
cmake --build ../../build/groups
ctest --test-dir ../../build/groups/products/calc-u-1600 -L longer
```

`docs/test-groups.md` at the repository root lists every test of both
products and its group.

## WebAssembly check

```sh
tools/build_wasm_check.sh
```

Builds `core/` + `app/` with Emscripten (`emcmake` on the repository
root, into `../../build/web`, the tree `frontends/web/build.sh` uses too)
and runs `tools/wasm_smoke.cpp` under node from the repo root: both models
boot through `shared::AppSession` and presets run cooperatively, a slice
per `tick()`, never blocking -- what a browser GUI will do. It keeps the
frontend-neutral layers portable ([Shared-Architecture.md](Shared-Architecture.md));
CI runs it in the `web` job. Needs `emcc`/`emcmake` (Homebrew
`emscripten`, or an activated emsdk; 6.0.13 matches the committed
`libsharpdx-wasm.a`, see `core/Basic/vendor/sharpdx/README.md`), node, and
the ROMs for the smoke run (without them it only builds).

## Web client

```sh
frontends/web/build.sh       # the WASM engine (target calcu_web) and frontends/web/dist/
frontends/web/run-tests.sh   # Playwright tests against frontends/web/dist/
```

Same Emscripten and ROMs as the WebAssembly check, plus node; the tests
install Playwright's Chromium and WebKit. Serve `frontends/web/dist/` over HTTP
(`python3 -m http.server -d frontends/web/dist`) to try it. CI's `web` job runs the
check, the build and the tests. See [Web-Client.md](Web-Client.md).

## Packaging

Release packaging (code signing, notarization, installers) is handled
by CI — see [`.github/workflows/calc-u-1600.yml (repository root)`](../../.github/workflows/calc-u-1600.yml (repository root))
and [`release.yaml`](../../release.yaml) (driven by the `release` command
from [ReleaseIt](https://github.com/tinue/ReleaseIt)) rather than duplicating
it here. The
Windows installer is currently unsigned; every other platform's package
is signed (macOS) or otherwise complete.
