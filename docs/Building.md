# Building

How to build Calc-U-59 and Calc-U-1600 from source: prerequisites, the
ROMs, the tests, the desktop apps, the Swift app and the web clients. For
how to *use* a product once it's built, see its help (`library/help/` and
`products/<product>/help/`, rendered at <https://calc-u.org>).

Everything here is derived from the repository's own recipes: the root
[`CMakeLists.txt`](../CMakeLists.txt), each product's `CMakeLists.txt`
and `frontends/qt6/CMakeLists.txt`, and the CI workflows
[`.github/workflows/calc-u-59.yml`](../.github/workflows/calc-u-59.yml)
and [`.github/workflows/calc-u-1600.yml`](../.github/workflows/calc-u-1600.yml),
which are also where to look first if a step below stops matching
reality. The Linux and Windows steps are transcribed from CI and not
verified on a fresh machine; if something's off, please open an issue
or PR.

All C++ builds go through the root CMake project, configured from the
repository root; no product directory configures on its own. Its options
choose what is built:

| Option | Default | What it adds |
|---|---|---|
| `CALCU_PRODUCT_59`, `CALCU_PRODUCT_1600` | ON | the product |
| `CALCU_FRONTEND_QT` | ON | the Qt6 apps (when Qt6 is found) |
| `CALCU_TESTS` | ON | the test programs and the Frequent CTest entries |
| `CALCU_TESTS_LONGER`, `CALCU_TESTS_CHECKPOINT` | OFF | the Longer and Checkpoint entries ([test-groups.md](test-groups.md)) |
| `CALCU_TESTS_DEBUG` | OFF | the cores and the tests at `-O0`, for stepping through |

The commands below run from the repository root and put their trees under
`build/`.

## Prerequisites

- **CMake** 3.21 or newer, and **Ninja**.
- A **C++20** compiler (plain portable C++; the long machine operations
  are C++20 coroutines, the library's `calcu::sessions::Task`): Xcode
  clang, GCC 11+, MSVC 2019 16.8+.
- **Python 3**: the help checks (`tools/help`) and the products' scripts
  among the tests.
- For the desktop apps, **Qt 6.3 or newer** with the `Widgets` and
  `Network` components, and for Calc-U-1600 also `Multimedia` (the
  buzzer's audio output). Older Qt6 (e.g. Ubuntu 22.04's packaged 6.2.4)
  doesn't provide `qt_standard_project_setup()` and won't configure. On
  macOS the Xcode command-line tools too: the Qt apps have Objective-C++
  sources (the library's `library/qt6/MacClipboard.mm` for the clipboard
  pictures, `library/qt6/MacHost.mm` for the host settings, Calc-U-1600's
  `screenshots/MacShotSupport.mm`).
- For the web clients, **Emscripten** (`emcc`/`emcmake`: Homebrew
  `emscripten`, or an activated emsdk) and **node**.
- For Calc-U-59's Swift app, **Xcode**.
- For Calc-U-1600, **zlib** for its tests (`zlib1g-dev` on Ubuntu; macOS
  has it) and `libsharpdx` (below).

## Get the source and the ROMs

```sh
git clone <this repository's URL>
cd calc-u
products/calc-u-59/roms/fetch_roms.sh
products/calc-u-1600/roms/fetch_roms.sh
```

The products need real ROM images, which aren't committed (each
product's `roms/README.md` gives provenance and licensing). Each
`fetch_roms.sh` downloads its product's full set into its `roms/` folder
and checks every file against its md5:

- Calc-U-59: the TI-59/TI-58 and TI-58C calculator ROMs and the Solid
  State ROM modules (24 files).
- Calc-U-1600: the 21 dumps and the CE-502B's module definition
  (`CE-502B.module.yaml`, which holds the module's ROM).

The builds bundle the set with the apps (the Qt6 app's resources, the
Web `dist/`). Skipping this step doesn't fail the build, but the built
product doesn't start a machine: at its start the library's ROM lookup
(`calcu::roms::lookUp`) checks every file of the set, and if one is
missing or bad the app names each such file instead of starting, and the
harness `calcu` ends at its start with exit status 2. Tests that need the
ROMs report themselves as skipped without them. On Calc-U-1600 no ROM set
is used in place of another (a missing Old ROM set does not fall back to
the New one); only the host-drive ROM (`firmware/pc1600-hostdrive/`, our
own) is bundled and read outside the lookup.

### `libsharpdx` (Calc-U-1600)

Every program loader of Calc-U-1600 (Load BASIC Program…, Load Machine
Code…, presets, the debugger's Build & Load) reads files through a
vendored Rust static library, `products/calc-u-1600/core/Basic/vendor/sharpdx/`:
it tells what a file is and tokenizes BASIC listings.

- On **macOS** it is committed: nothing to do.
- On **Linux and Windows** fetch it before configuring, from a
  SharpDataExchange GitHub release:
  `products/calc-u-1600/tools/fetch_sharpdx.sh`.
- Under **Emscripten** the committed `libsharpdx-wasm.a` is used; it was
  built with Emscripten 6.0.13 (`core/Basic/vendor/sharpdx/README.md`).

CMake stops with an error if the library is missing. No Rust toolchain is
needed.

## The tests

```sh
tools/run_tests.sh calc-u-59            # every Frequent test of the product
tools/run_tests.sh calc-u-1600 ce158    # the TESTs whose name contains ce158
```

The repository's one test script configures the root project without the
Qt apps into `build/tests` (Debug the first time, so the cores and the
tests compile at `-O1`), builds the product's test programs and the
harness `calcu`, and runs every Frequent CTest entry of the product: its
runner programs (`CalcU59Tests`; `CalcU1600CoreTests`,
`CalcU1600SessionTests`, `CalcU1600AppSessionTests`, `CalcU1600DapTests`,
`CalcU1600HelpTests`), the help and preset checks and the source checks.
An argument is a filter for the runner programs. The tests run in the
product directory, since they load `roms/` and their fixtures by relative
paths.

The tests use the repository's one C++ test runner,
`library/testing/TestHarness.hpp`: each file registers its tests with
`TEST(name)` and checks with `CHECK` / `CHECK_EQ`; a test that needs the
ROMs starts with `SKIP_UNLESS(...)`. The runner's `main()` (target
`calcu_test_runner`) runs every registered test; there is no list to
keep.

Both products and the library in one tree, as a plain `ctest` (the
Frequent group):

```sh
cmake -S . -B build/tests -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCALCU_FRONTEND_QT=OFF
cmake --build build/tests
ctest --test-dir build/tests -j8
```

This is the script's tree: `tools/run_tests.sh` configures `build/tests`
as Debug only when it is missing, and uses a tree configured by hand as
it is.

CI's `core-tests` jobs run the same with one product each
(`-DCALCU_PRODUCT_1600=OFF` or `-DCALCU_PRODUCT_59=OFF`).

The cores and the tests compile at `-O1` in a Debug build (the root
`CMakeLists.txt`'s `calcu_debug_o1`): the suites run about three times
faster than at `-O0`. To step through a failing test, build a separate
full-debug tree (in CLion: add the option to a CMake profile's options):

```sh
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCALCU_FRONTEND_QT=OFF -DCALCU_TESTS_DEBUG=ON
cmake --build build/debug --target CalcU1600CoreTests
(cd products/calc-u-1600 && ../../build/debug/products/calc-u-1600/CalcU1600CoreTests)
```

### Test groups

Every CTest entry carries a group label: `frequent` (on every change,
≤ 60 s per product), `longer` (before merging into `main`, ≤ 10 min:
Playwright, the WebAssembly checks, the reference runs, the screenshot
scenarios, the DAP smoke runs) and `checkpoint` (before a release). A
tree has only the Frequent group unless it is configured with
`-DCALCU_TESTS_LONGER=ON` or `-DCALCU_TESTS_CHECKPOINT=ON`; some entries
also need the Qt apps in the tree, Playwright a built
`frontends/web/dist/`:

```sh
cmake -S . -B build/groups -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCALCU_TESTS_LONGER=ON -DCALCU_TESTS_CHECKPOINT=ON
cmake --build build/groups
ctest --test-dir build/groups/products/calc-u-1600 -L longer
```

[`test-groups.md`](test-groups.md) lists every entry and its group.

**Checking presets.** `calcu_preset_check.<name>` runs
`calcu --product <c59|c1600> check-preset` over the product's presets;
by hand: `calcu --product c1600 check-preset <file>…`.

## The Qt6 desktop apps

```sh
tools/run_qt.sh 59 --build      # or 1600
tools/run_qt.sh 1600            # builds and starts it
```

`tools/run_qt.sh <59|1600> --build` configures the root project once
with the one product (`build/app-59` or `build/app-1600`, Release) and
builds the app; `--app-dir` prints the app's directory, and a file
argument opens it in the started app. The app is in
`build/app-<n>/products/calc-u-<n>/frontends/qt6/`.

### macOS

```sh
brew install qt cmake ninja
tools/run_qt.sh 1600 --build
```

Homebrew's Qt is found automatically, and the app is a real `.app`
bundle. To run an app outside this checkout (e.g. on another Mac), embed
the Qt frameworks:

```sh
$(brew --prefix qt)/bin/macdeployqt "$(tools/run_qt.sh 1600 --app-dir)/Calc-U-1600.app"
```

Without that step the app still runs from the build directory (Qt is
found via the Homebrew prefix), just not as a relocatable bundle.
Signing, notarizing and the DMG are release packaging:
[macos-signing.md](macos-signing.md).

### Linux (CI-derived)

```sh
sudo apt-get install -y \
  build-essential cmake ninja-build patchelf zlib1g-dev \
  qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev qt6-wayland libgl1-mesa-dev \
  qt6-gtk-platformtheme qt6ct
products/calc-u-1600/tools/fetch_sharpdx.sh
tools/run_qt.sh 1600 --build
```

(Calc-U-59 needs neither `qt6-multimedia-dev`, `zlib1g-dev` nor
sharpdx.) This produces a plain executable plus a `resources/` folder
that must stay next to it. That's enough to run on the machine that built
it. The standalone `.AppImage` (`linuxdeploy` and its Qt plugin, Wayland
plugin handling included) is built by the workflows' `linux` jobs; copy
their steps.

### Windows (CI-derived)

```powershell
# Qt 6.8.x (MSVC, win64_msvc2022_64; for Calc-U-1600 with the qtmultimedia
# module) and Ninja by whatever means you prefer (CI uses the
# aqtinstall-based jurplel/install-qt-action).
cmake -S . -B build\app-1600 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCALCU_PRODUCT_59=OFF
cmake --build build\app-1600
```

Run this from a shell with the MSVC toolchain on `PATH` (a Visual Studio
"Developer Command Prompt", or `vcvarsall.bat`/`ilammy/msvc-dev-cmd`'s
equivalent); Calc-U-1600 needs `products\calc-u-1600\tools\fetch_sharpdx.sh`
first (a POSIX sh script: run it with Git's bash). This produces
`Calc-U-1600.exe` plus a `resources\` folder that must stay next to it.
The packaging jobs configure the root into `build/release` with
`-DCALCU_TESTS=OFF` (no tests, so no zlib on the Windows runners) and the
other product off, run `windeployqt --release --compiler-runtime` on the
`.exe` and package the staged folder with Inno Setup
(`frontends/qt6/resources/windows/calc-u-<n>.iss`, which includes the
generated `Version.iss` from that tree); see the `windows` jobs for the
exact staging layout. The installers are unsigned.

CI also builds native ARM64 installers (`windows-arm64`) on a
`windows-11-arm` runner, the same way with Qt's `win64_msvc2022_arm64`
kit and `ilammy/msvc-dev-cmd`'s `arm64` toolchain (Calc-U-1600: the
`windows-aarch64` sharpdx release). `ISCC.exe /DAPP_ARCH=arm64` makes the
`.iss` produce `Calc-U-<n>-Setup-arm64.exe`. To build it yourself on a
Windows ARM64 machine, run the steps above from an ARM64 developer prompt
(`vcvarsall.bat arm64`) with the ARM64 Qt kit.

## Calc-U-59's Swift app (iOS, iPadOS)

The Swift app is the iPhone and iPad app; Calc-U-59's Mac app is the Qt6
app above. Open `products/calc-u-59/frontends/swift/Calc-U-59.xcodeproj`
in Xcode, or:

```sh
xcodebuild -project products/calc-u-59/frontends/swift/Calc-U-59.xcodeproj \
  -scheme Calc-U-59 -destination 'generic/platform=iOS Simulator' build
```

The Swift app links `CalcU59.xcframework`, which
`frontends/swift/build_xcframework.sh` builds from the root project's
`calcu59_core` and `calcu59_shared`; Xcode compiles only Swift and the
bridge. The project carries the author's Apple Developer Team ID and
bundle identifier (`ch.erzberger.calcu59`). To build it yourself, change
the team on the target's *Signing & Capabilities* tab to your own; Xcode
updates the bundle identifier. The Simulator needs no changes. For iCloud
card sync on a real device, also register a new iCloud container and
update the two container identifiers in `Calc-U-59.entitlements`.

## The web clients

```sh
products/calc-u-59/frontends/web/build.sh       # or calc-u-1600
products/calc-u-59/frontends/web/run-tests.sh   # Playwright against frontends/web/dist/
```

`build.sh` compiles `core/` and `app/` with Emscripten (`emcmake` on the
repository root, into `build/web`; the engine targets `calcu59_web` and
`calcu1600_web`) and builds a self-contained `frontends/web/dist/` that
the website embeds; `run-tests.sh` installs Playwright's Chromium and
WebKit and runs the product's tests against it. Serve `dist/` over HTTP
(`python3 -m http.server -d products/calc-u-1600/frontends/web/dist`) to
try it. CI's `web` jobs build and test it and upload it zipped. Each
product's `docs/developer/Web-Client.md` describes its client.

Calc-U-1600 also has a WebAssembly check:
`products/calc-u-1600/tools/build_wasm_check.sh` builds `core/` and `app/`
in `build/web` and runs `tools/wasm_smoke.cpp` under node: the library's
app session with Calc-U-1600's part, in virtual time, starts a machine,
runs presets through their commands and types. It keeps the
frontend-neutral layers portable; CI runs it in the `web` job. Without
the ROMs it only builds.

## CLion and other IDEs

Open the repository root in CLion (or any CMake-aware IDE) and it
configures the root project with every product: the Qt6 apps, the
harness `calcu` and the test programs as CTest targets. The repository's
`.run/` holds shared run/debug configurations:

| Run configuration | CMake target | Notes |
|---|---|---|
| `Calc-U-1600` | `CalcU1600Qt` | Calc-U-1600's Qt6 app |
| `CalcU1600CoreTests` | `CalcU1600CoreTests` | Calc-U-1600's main test program |
| `calcu-59` | `calcu` | the harness, `calcu --product c59 serve`: one command per line on standard input |
| `calcu-1600` | `calcu` | the harness, `calcu --product c1600 serve` |

Each runs in its product directory, so the ROMs are found. All of them
run and debug interactively (breakpoints, variable inspection, stepping).

The sources of each product's `core/` and `app/` go into two static
libraries per product (`calcu59_core` / `calcu59_shared`,
`calcu1600_core` / `calcu1600_shared`, in the product's
`CMakeLists.txt`). Calc-U-1600's are globbed: a new `.cpp` needs no list
entry, re-run CMake (or let Ninja's glob check do it); Calc-U-59's core
sources are listed in its `CMakeLists.txt`.

## Packaging

Release packaging (installers, AppImages, the web and help zips, Mac
signing and notarization) is done by ReleaseIt's `release` command
([ReleaseIt](https://github.com/tinue/ReleaseIt)) from each product's
`release.yaml`, which dispatches the product's workflow
([Process.md](Process.md)). Both Mac apps are built, signed and
notarized in CI ([macos-signing.md](macos-signing.md)); Calc-U-59's iOS
app is built locally by Xcode (`release ios`). The Windows installers are
unsigned.
