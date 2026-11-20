# Test groups

Every CTest entry of the repository, its group and what it runs. The C++
tests of the library and both products use one runner,
`library/testing/TestHarness.hpp`; a test program run with a name as its
argument runs only the `TEST`s whose names start with it.

## How the groups run

| Group | CTest label | Budget | Registered in a tree | Runs |
|---|---|---|---|---|
| Frequent | `frequent` | ≤ 60 s per product, locally | always (with `CALCU_TESTS`) | on every change, locally and in CI |
| Longer | `longer` | ≤ 10 min | with `-DCALCU_TESTS_LONGER=ON` | locally before merging into `main`; Playwright and the WASM smoke also in CI (the `web` jobs, in Microsoft's Playwright container) |
| Checkpoint | `checkpoint` | none | with `-DCALCU_TESTS_CHECKPOINT=ON` | before a release |

A plain `ctest` in a tree without the two options is the Frequent run, so
the CI jobs need no filter. The library and each product register their
entries with `calcu_add_test(<name> <group> COMMAND … [WORKING_DIRECTORY
<dir>] [PROPERTIES …])` (the root `CMakeLists.txt`): the group is the
label, a Longer or Checkpoint entry is registered only with its option,
and the entry runs in the product directory unless it names another (the
library's own entries run in their build folder). The source checks both
products run (`<Product>NoOwnRomLookup`, `<Product>NoPlatformSettings`)
come from `calcu_product_source_checks()` (`library/CMakeLists.txt`); the
help entries (`<Product>HelpCheck`, `<Product>HelpBuild`) from
`calcu_add_help_tests()` (the root `CMakeLists.txt`); the preset checks
(`calcu_preset_check.<name>`) from `calcu_add_preset_check()`
(`library/presets/PresetChecks.cmake`).

A tree with both options and the Qt apps:

```sh
cmake -S . -B build/groups -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCALCU_TESTS_LONGER=ON -DCALCU_TESTS_CHECKPOINT=ON
cmake --build build/groups
ctest --test-dir build/groups -L longer        # or -L frequent, -L checkpoint
ctest --test-dir build/groups/products/calc-u-1600 -L longer   # one product
ctest --test-dir build/groups -N -L longer     # list the entries only
```

Prerequisites outside the tree: the products' Playwright entries test a
built `frontends/web/dist/` (`frontends/web/build.sh`); the library's
builds its test product itself (Emscripten, `build/web`); the WASM smoke
builds `build/web` with Emscripten itself; the XCUITest entries need the
simulators of `bin/simulators.yaml` (`bin/simulators setup`); the
Calc-U-1600 screenshot and DAP entries need the Qt app in the tree.
Run the Longer group without `-j`: the products' Playwright entries and
their ROM-missing builds share ports 8059/8060 and `build/web`, and the
XCUITest entries share the simulators.

A test that outgrows its group's budget moves up a group or is split.

The Longer group measured 291.5 s on an M4 Pro, within its 10 min.

## Local Linux CI

`ci/linux/run.sh` runs in Podman on Ubuntu 24.04, natively on arm64, what
can fail on GitHub's Linux runners and not on the Mac: GCC 13 and libstdc++
(a missing include, a stricter overload, an internal compiler error, the
order of argument evaluation at run time), Ubuntu's Qt and its `moc`, the
web jobs' pinned Emscripten and Playwright in Linux browsers. These caused
the failed GitHub runs; what gives the same result on every host (the help,
the source checks) is covered by the Mac's Frequent run, and Apple's
notarisation cannot be tried locally. It builds no release asset. It runs
before a GitHub run is planned (a pre-release, for example), not after every
merge. Exceptions: work on the pipeline itself (the workflows, `ci/`), and
work with a high risk of breaking the pipeline build (the build system, the
toolchain, platform-specific code). A change is green by the build and the
Frequent group.

| Job | What | |
|---|---|---|
| `gcc` | one tree (RelWithDebInfo) with both products, the Qt frontends and the tests: the code the workflows' `library`, `core-tests` and `linux` jobs build, then the CTest entries those jobs run, without the Qt window tests (`calcu_qt6_*`, `CalcU59QtTests`, `CalcU1600QtTests`), which no Linux job runs | default |
| `web-59`, `web-1600` | `web` up to the tests, each product's Emscripten, in the web image: the `web` jobs' Playwright container with their build tools; `web-1600` also runs the library's Web shell tests | when named |

`ci/linux/run.sh web-1600` runs one job. The working tree is copied into a
volume without what git ignores (but the ROMs), so builds never write into
the checkout. The image follows `ci/linux/Containerfile`, which mirrors the
workflows' install steps and changes with them; its web image starts from
the same Playwright container as the `web` jobs. Measured on an M4 Pro
(machine `calcu` with 12 CPUs and 16 GB), the `gcc` job: 3.8 min with
a new build tree, 35 s warm (most of it the tests).

## Library

Measured on an M4 Pro, RelWithDebInfo.

| CTest entry | What | Group | Measured |
|---|---|---|---|
| `calcu_library_tests` | The library's own tests, `library/<area>/tests/*.cpp` (about 1000 `TEST`s), with the machines built for the tests (`library/machine/tests/TestMachines.*`) and no product code; also run alone by the CI job `library` of both product workflows. The pacing and session tests run on a scripted clock (`library/pacing/tests/TestSliceRunner.hpp`, `library/sessions/tests/ScriptedClock.hpp`), never the host's. Rigs per area: the settings tests' product (`settings/tests/TestProduct.hpp`); the ROM lookup's temporary ROM files (`roms/tests/TestRomFiles.hpp`); the harness's test product (`harness/tests/TestProduct.*`: `test-seg`, `test-dot`, the test printers TD-20 and TP-4) in virtual time, each run in a temporary folder; the start-up rig (`startup/tests/TestProductMachines.hpp`, with the stand-in loader `TestPresetLoader.hpp`); the typing rig (`typing/tests/TypingRig.hpp`, `test-type`); the preset rig (`presets/tests/PresetRig.hpp`, the fixtures of `presets/tests/fixtures/`: `bad/`, one file per class of error, `bundle/`, `good/`); the paper kit (`paper/tests/PaperTestKit.hpp`, the fixtures of `paper/tests/fixtures/`: three `.marks` and five reference pictures compared byte for byte, `CALCU_PAPER_PICTURES=1` writes them anew); the media fixture (`media/tests/MediaFixture.hpp`, the media test catalogue, a save folder and a bundled folder per test); the opening stand-ins (`opening/tests/TestOpeners.hpp`). The host links are tested with an injected link factory on every platform. 21 of the `TEST`s are harness attach (`harness/attach/tests/`), built with `CALCU_ATTACH` (on by default in a POSIX tree with the tests): real sockets on 127.0.0.1 and a host loop on the real clock, about 3 s | Frequent | 0.64 s (most of it the five-hour scripted pacing runs) |
| `calcu_library_pty_tests` | 4 `TEST`s of `library/peripherals/tests/pty_tests.cpp`: the host links on real pseudo-terminals for serial links, in a temporary folder; a program of its own, registered only where `CALCU_HAS_PTY` is set (macOS, Linux), so that it can move to Longer alone | Frequent | 0.8 s |
| `calcu_cli.<case>` | `calcu_test` (the harness with the test product only) as a process of its own, run by `library/harness/tests/cli/cli_cases.cmake` with `cmake -P`: argv, standard input and output, exit status. Eight entries: `product_by_name_or_short_form_any_case`, `one_shot_runs_in_order_and_exits_zero`, `first_refusal_exits_nonzero_with_reason`, `serve_one_reply_per_line_until_quit`, `serve_ends_at_end_of_input`, `unknown_product_lists_products`, `check_preset_exit_status`, `refused_expect_ends_one_shot_nonzero` | Frequent | 0.1 s, the eight together |
| `calcu_preset_check.LibraryPresetFixtures` | `calcu_test check-preset` over `library/presets/tests/fixtures/good/*` and a template filled in by `fixtures/fill.cmake` (run by `RunPresetCheck.cmake` with `cmake -P`); fails when a file fails | Frequent | 0.05 s |
| `calcu_serve_python` | `library/harness/python/tests/test_calcu_serve.py`: the Python helper for `calcu serve` against `calcu_test`, with its reply timeout (with `uv run --no-project python -I`, else Python 3) | Frequent | 0.05 s |
| `LibraryPlatformStoreCheck` | `library/settings/tests/check_platform_stores_test.sh`: the self-test of `library/settings/check_platform_stores.sh`, the source check the products run over their frontends; not registered on Windows | Frequent | 0.03 s |
| `LibraryShellNoProduct` | `library/qt6/tests/check_no_product_code.sh`: no include from `products/` and no model, peripheral or product name in the code (comments aside) of `library/shell`, `library/qt6` and `library/web/components`; not registered on Windows | Frequent | 0.2 s |
| `LibraryHelpTool` | `python3 -m unittest discover -s helptool/tests -t .` in `tools/`: the help tool's cases (`tools/helptool/tests/test_*.py`: the manifests, the asset and `toc.json`, anchors against `library/help/anchor_cases.txt`, links and `chapter:` links, the checks), each on a copy of the fixture help tree `tools/helptool/tests/fixtures/good/`; registered when Python 3 is found | Frequent | 0.5 s |
| `LibraryWebNodeTests` | `node --test` over `library/web/tests/build.test.js` (the Web frontend shell's build functions, `library/web/build/dist.js`: MANIFEST, `files.json`, the service worker stamp, one global in `calc.js`, the CSS scope check, the version from `release.yaml`), `shell_model.test.js` (the React set's shell models, `library/web/components/shellModel.js`, the mirror of the C++ model cases under the same names, on `library/shell/tests/inputs.txt` and `hit_cases.txt`), `host_keys.test.js` (`library/web/components/hostKeys.js`) and `embed_props.test.js` (`library/web/src/embed.js` `embedProps`); registered when `node` is found; also run by the CI job `library` | Frequent | 0.3 s |
| `calcu_qt6_tests` | The Qt6 component set's test program (`calcu_qt6`, `library/qt6/tests/*_tests.cpp`, `QtTestMain.cpp`), `QT_QPA_PLATFORM=offscreen`: file opening (`FileOpening`, `AskedSend`), host keys, the paper panel and `ClipboardService` (an app session with PaperPart and the test printers), the media picker and the name-and-save dialog (without a window), the run-control buttons (a held step's real 250 ms repeats), the shell's widgets on the test window `tests/TestWindow.*` over `TestShellPart` and SettingsPart on a scripted clock (the shortcut and touch cases with Qt Test, skipped without it), harness attach in `SessionHost` (real sockets on 127.0.0.1 and the 60 Hz timer, about 1 s; only with `CALCU_ATTACH`). Registered only in a tree with `CALCU_FRONTEND_QT` where Qt6 Widgets is found (not in `build/tests`) | Frequent | 2.7 s (1.8 s of it the paper toast's real wait, about 0.6 s the attach host on the real clock) |
| `calcu_qt6_scenarios` | The 9 `TEST`s of `library/qt6/tests/shell_scenarios.cpp`, the shell's scenarios on the test window (`menus_and_their_items`, `reset_shortcut_ctrl_r`, `platform_menu_roles`, `control_bar_controls`, `settings_dialog_sets_the_model`, `loading_after_500_ms_and_reset_hides_it`, `notice_shown_and_ok_closes_it`, `help_opens_the_product_address`, `key_under_the_pointer`), widgets found by their object names; offscreen; registered with `calcu_qt6_tests` | Frequent | 0.2 s |
| `LibraryPlaywrightVersion` | `tools/check_playwright_version.sh`: one Playwright version in the three `package-lock.json` files (`@playwright/test`, `playwright`, `playwright-core`), the Playwright container of both product workflows' `web` jobs and the local CI's web image (`ci/linux/Containerfile`); reads files only; not registered on Windows | Frequent | 0.05 s |
| `LibraryWebPlaywright` | `library/web/tests/run-tests.sh`: builds the product made for the tests (`library/web/tests/product/build.sh`: `calcu_web_test_product` in `build/web`, then `Test-A`, `Test-B`, a second release of `Test-A` and `Test-M` (with MediaPart) in `build/web/library/web/test-dist/`), then Playwright over `library/web/tests/*.spec.js`: `hosting`, `storage`, `embed` in Chromium (root, sub-path) and WebKit (root); `offline` in Chromium and WebKit; `prerender` in Node; `paper` (the React paper panel with the test printers), `run-control` (the React run-control buttons; an ended trace as a download), `shell` (the React set's shell on the test product with `shell=1`) and `media` (the React media components on `Test-M`) in Chromium and WebKit. 140 passed, 2 skipped. Not registered on Windows; in CI in the `web` job of both product workflows, with each one's Emscripten | Longer | 35 s, plus 10 s for the build with `build/web` built |

## Calc-U-59

Measured on an M4 Pro, RelWithDebInfo.

| CTest entry | What | Group | Measured |
|---|---|---|---|
| `CalcU59Tests` | `products/calc-u-59/tests/*.cpp`, about 600 `TEST`s: unit and ROM-driven integration tests of the core and the app layer: the machine interface and descriptors, the ROM lookup, the PC-100C and the module slot, the settings file, the golden replays through the machine interface (`golden_runs_tests.cpp`, `CALCU59_GOLDEN_RECORD=1` records a missing reference only), the harness product entry and its `c59` commands, the machine state, the app session and the session thread, the product's flows on it (`SessionRig.hpp`, `PartRig`) and the Web part, typing, presets (`tests/AppRig.hpp`, `tests/TIPresetRig.hpp`), the constant memory file, magnetic cards and the card reader (`tests/CardRig.hpp`, the ROM-written fixtures `tests/fixtures/cards/*.hex`), media, opening, the debug panel and the ASM overlay (`tests/fixtures/asm/*.words`), the debugger and the trace recorder, the paper of the PC-100C and its paper replies (`CALCU59_PAPER_REPLIES=1` writes the committed `.paper-text` and `.paper-marks` anew), the shell's models with the Swift frontend's traits, the help's settings keys and names (`help/settings-keys.txt`, `help.yaml`'s `names:`, the tutorial presets), the cases of `reference/Idle.md` through the harness (`idle_tests.cpp`). A `TEST` that needs ROMs that are missing reports `SKIP`. The reference runs and the real-clock pacing run of this program return at once without their environment variables (the Longer and Checkpoint entries below set them) | Frequent | 32 s alone, 41 s in `ctest -j8` |
| `CalcU59TraceReader` | `tools/read_trace.py --json` reads the trace `CalcU59Tests` wrote (`build/trace-test.bin`); after `CalcU59Tests` (`DEPENDS`); its 4 MB of JSON go to `/dev/null` (`sh -c`; CTest took 15 s to take the output in), a failure shows on standard error and in the exit status | Frequent | 0.1 s |
| `CalcU59QtTests` | `products/calc-u-59/frontends/qt6/tests/`, 27 `TEST`s on the real window of Calc-U-59 put together from the library's shell (`tests/QtFixture.*`: the window over the session on the ROMs compiled in as the app has them, the product's `examples/` as the bundled examples, the file dialogs and the browser replaced by recorders; on the real clock): the menus, shortcuts and roles, the bar per model, the settings dialog, the three columns (the window's picture into `test-pictures/` of the build tree), the key under the pointer, a press on the fitted card, display hold at full speed, the window without ROMs, a bundled example with its cue card, the debug panel, the card reader's buttons and dialog, the paper, harness attach on port 0 (with `CALCU_ATTACH`); `QtTestMain.cpp`, `QT_QPA_PLATFORM=offscreen` and a `HOME` of the build tree's (no settings of the user's); registered only in a tree with `CALCU_FRONTEND_QT` where Qt6 Widgets is found | Frequent | 22 s (about 1 s of ROM start-up per window) |
| `CalcU59NoOwnSession` | `tests/check_no_own_session.sh`: the Qt6, Swift and Web frontends use no `EmulatorSession`, `EmulationRunner`, `TISessionController` or own `SessionController`; no `std::thread` in `app/` but `SessionThread`; the Swift bridge includes no `SessionController.hpp` and calls no tick; Qt6 sends no suspend or continue | Frequent | 0.03 s |
| `CalcU59FrontendsDrawControlState` | `tests/check_frontends_draw_control_state.sh`: no file of the Qt6, Web and Swift frontends reads `hasCardReader`, `cu59::info(`, `c59.model`, `c59.web.model`, `allModules` or `MachineModel`, and `App/MachineModel.swift` does not exist: they draw the model, the module, the printer and the card controls from the control state | Frequent | 0.02 s |
| `CalcU59HelpCheck` | `tools/help check calc-u-59 --quiet-warnings`: the help tool's checks over `library/help/` and `help/`; registered when Python 3 is found | Frequent | 0.3 s |
| `CalcU59HelpBuild` | `tools/help build calc-u-59 --quiet-warnings --out <tree>/products/calc-u-59/help`: the help asset `Calc-U-59-help.zip`, so the test suite builds it too | Frequent | 0.4 s |
| `CalcU59CoreSeam` | `tests/check_core_seam.sh`: no core-internal header outside `core/` and `tests/`, no machine call in the adapters | Frequent | 0.02 s |
| `CalcU59OpenWithTypes` | `tools/check_open_with_types.sh`: the Qt6 app's Open With offers the three preset extensions on macOS (`Info.plist.in`), Windows (`calc-u-59.iss`) and Linux (`calc-u-59.desktop`) | Frequent | 0.03 s |
| `CalcU59NoOwnRomLookup` | `library/roms/tests/check_no_own_rom_lookup.sh`: no product source outside `app/ProductRoms.*`, the catalogue and the Qt6 bundle looks for a ROM file | Frequent | 1.2 s |
| `CalcU59NoPlatformSettings` | `library/settings/check_platform_stores.sh` over `frontends/` with `tools/platform_stores_allowed.txt`: no setting in UserDefaults, `@AppStorage`, QSettings or `localStorage`; allowed: the trace folder's bookmark and the Playwright check that 2.3.0's entries are not read; not registered on Windows | Frequent | 0.02 s |
| `CalcU59HarnessRomMissing` | `tests/harness_rom_tests.py`: `calcu --product c59` on a ROM folder without one file (`CALCU59_ROMS_DIR`) ends with exit status 2 naming it; on the full set it starts (with Python 3) | Frequent | 0.3 s |
| `CalcU59NoOldTools` | `cmake -P tools/check_no_old_tools.cmake`: `tools/headless/`, `tools/agentic/`, `core/TIAdapters.*` and `tests/headless_rom_tests.py` do not exist; no source names `ti59headless`, `TI59HEADLESS_LIB`, `TIMemoryAdapter`, `memoryOf(` or includes `TIAdapters.hpp` | Frequent | 0.05 s |
| `CalcU59CalcuRuns` | `cmake -DCALCU=… -P tests/calcu_runs.cmake`: `calcu --product c59 wait 1s then display-text` as a process, exit status 0 and the display; also a step of every desktop job of `calc-u-59.yml` | Frequent | 0.1 s |
| `CalcU59CalendarScripts` | `tools/calendar/selftest.py --calcu <calcu>`: `tools/calendar/fastcal.py` regenerates `calendar-09` equal to the tracked file, and `years.py` finds the year in it through `calcu serve` (with Python 3) | Frequent | 2 s |
| `calcu_preset_check.CalcU59Presets` | `calcu --product c59 check-preset` over the examples (with the four that carry magnetic cards) and the three startup presets of `examples/` | Frequent | 0.03 s |
| `CalcU59XcodeProjectCheck` | `frontends/swift/check_xcode_project.sh`: Xcode compiles only Swift and the bridge and links `CalcU59.xcframework`; the app bundles the 24 ROM files of the full set; only Debug takes incoming connections, with `Calc-U-59-Debug.entitlements`; the XCFramework script phase declares its output | Frequent | 0.02 s |
| `CalcU59NoAttachSources` | `frontends/swift/check_no_attach.sh --sources`: attach only for the Debug `macos` and `ios-simulator` slices in `build_xcframework.sh`, the bridge's and the Swift call sites under the Debug and platform guard. The `--binary` half runs as a build phase of every Release and device build of the app | Frequent | 0.02 s |
| `CalcU59NoTestAids` | `frontends/swift/check_no_test_aids.sh`: the Swift app's sources hold no `--UITesting`, `isUITesting`, `btn-key-rs`, `keystroke-playback-status`, `--setting`, `--module`, `isKeystrokesPlaying` or `MachineModel` | Frequent | 0.02 s |
| | **Frequent, Calc-U-59** | | 31.2 s wall alone with `-j8`, `CalcU59Tests` the longest |
| `CalcU59WebPlaywright` | `frontends/web/run-tests.sh`: Playwright over `frontends/web/tests/*.spec.js`, Chromium (root, sub-path) and WebKit (offline); the servers are `library/web/tools/serve.py`, the offline server and the prerender sandbox `library/web/tests/dist-helpers.js`, `gotoReady` in `tests/helpers.js`: the page on the library's shell (the three layouts, their screenshots in the test output), the session, storage, host keys, files through `open`, the debug panel, the cards, the window without ROMs (`rom-missing.spec.js`). 149 passed, 5 skipped | Longer | 78 s |
| `CalcU59ReferenceRuns` | `CalcU59Tests reference_runs_of_the_examples` (`tests/reference_runs_tests.cpp`) with `CALCU59_REFERENCE_COMPARE=1`: each upgraded example through the harness entry in virtual time (`preset <file>`, then the bare `wait`), compared with the reference run of its 2.3.0 file (`tests/reference/v2.3.0-examples/`) in the display, the partition, the raw RAM (120 registers) and the paper (`paper text`, `paper marks`); ticks, `Elapsed:`, the log and the `playback:` line are not compared; `historic/life.ti59` is only loaded; the four card examples also in the banks of their in-file and written cards. A difference is a finding, named with its reason in `kFindings`, never a new reference. `CALCU59_REFERENCE_EXAMPLES=<folder>` compares a draft folder; `CALCU59_REFERENCE_RECORD` writes to `build/reference-runs/`, never into the references | Longer | 7 s |
| `CalcU59RomMissingBuild` | `tests/rom_missing_build.sh`: `roms/` without one file (default `TMC0583.txt`); the harness `calcu` on it (`CALCU59_ROMS_DIR`) and a Web `dist/` built from it start without a calculator and name the file; not registered on Windows | Longer | 3 s (the Web build tree already built) |
| `CalcU59Scripts` | `tests/check_scripts.py`: `tools/coverage.py build boot digit5 add` and `show` give 2.3.0's words of the three scenarios (`tests/reference/scripts/coverage-base.json`) without the key-detect range; `tools/fastmode_matrix.py label` gives 2.3.0's verdicts of the `label` cases (`fastmode-label-base.txt`) (with Python 3) | Longer | 6 s |
| `CalcU59UIRegression` | XCUITest plan `UIRegressionTests` on the `iphone` simulator, every launch with `--settings-file`: the regression tests, `AttachTests` (harness attach on port 59160: Home and back, full speed by a press on the display), `FamilyStructureTests` (the shell's menus, bar, More and settings; attach on port 59162), `DebugPanelTests`; `PacingTests` and `ScreenshotTests` skipped | Longer | 447 s, 39 tests (iPhone 18 Pro Max, iOS 27.2) |
| `CalcU59SwiftUnitTests` | `xcodebuild test -testPlan UnitTests` on the `iphone` simulator: `Calc-U-59Tests`, hosted by the app with `--settings-file unit-tests.ini`, a second session through the real bridge (`MessageOrderTests`, `HostServicesTests`, `SuspendTests`, `FrameGapTests`, `OpeningTests`, `ShellBridgeTests`) | Longer | 14 s of tests, about 40 s with the build |
| `CalcU59NoAttachInRelease` | `frontends/swift/check_no_attach.sh`: builds the Release Mac app (signed) and the Debug iOS device app (unsigned) into `build/xcode-no-attach`; no attach in them, in the Release XCFramework slices and the Debug `ios` slice; the Release app's entitlements | Longer | 25 s with the XCFramework trees built, minutes from nothing |
| `CalcU59PacingRealClock` | `CalcU59Tests pacing_longrun_real_clock` (`pacing_longrun_real_clock_ti` of `tests/pacing_longrun_after_tests.cpp`, the library's pacer) with `CALCU59_PACING_LONGRUN=600` | Checkpoint | 10 min (by design) |
| `CalcU59WebPacing` | `frontends/web/run-tests.sh tests/pacing.spec.js --project root` with `CALCU59_WEB_PACING=1`: a TI-59 keeps one minute in the Web frontend, measured by the session's `ticks` request (wall 60.009 s, emulated 59.994 s) | Checkpoint | 62 s |
| `CalcU59QtPacingProbe` | The Qt6 app (`CALCU_FRONTEND_QT`) with `--pacing-probe 60`, offscreen, a temporary `HOME` in the tree | Checkpoint | 61 s |
| `CalcU59PacingIOS` | XCUITest plan `PacingCheckpoint` on the `iphone` simulator: `PacingTests.testTI59KeepsOneMinute`, `ticks` over attach on port 59161 | Checkpoint | 67 s |
| `CalcU59MacAttach` | `tests/mac_attach_check.sh <calcu>`: the Debug Mac app through Launch Services on port 59171; `calcu attach`; one minute in front, 30 s covered by a grey window over every screen (`tests/mac_window.swift`), hidden and shown | Checkpoint | about 2 min with the app built |
| `CalcU59SwiftUnitTestsMac` | `xcodebuild test -testPlan UnitTests` on macOS (the macOS host services) | Checkpoint | 14 s of tests |
| `CalcU59AppStoreScreenshots` | `bin/run-screenshots`: the App Store set (the iPhone Duo is opened flat by hand) | Checkpoint | not measured |
| `CalcU59UITestsMac` | XCUITest plan `UIRegressionTests` on macOS | Checkpoint | not measured (needs a signed runner and the Accessibility permission) |

## Calc-U-1600

Measured on an M4 Pro, RelWithDebInfo unless noted.

| CTest entry | What | Group | Measured |
|---|---|---|---|
| `CoreTests` | The `TEST`s of `products/calc-u-1600/tests/` and `app/tests/` that do not use `app/tests/SessionFixture.hpp`, about 800: unit and integration tests of the core (the CPUs, the machines, every peripheral, the display, tape and disk, the inspector, disassembly) and the app layer (real ROMs, presets, loaders and program files, BASIC typing, media kinds, opening, the paper of the plotters and their paper replies (`CALCU1600_PAPER_REPLIES=1` writes the committed `*.<plotter>.paper-marks` anew), the debug target and run control, the DAP's layer, settings, the ROM lookup), the machine interface and descriptors, the golden replays through it (`golden_runs_tests.cpp`, `CALCU1600_GOLDEN_RECORD=1` records new references only), the harness product entry. A `TEST` that needs ROMs that are missing reports `SKIP`. The example references and the real-clock pacing runs return at once without their environment variables (the Longer and Checkpoint entries below set them) | Frequent | 26.5 s in `ctest -j8` |
| `CalcU1600SessionTests` | The `TEST`s of the files of `tests/` that use `SessionFixture.hpp` (a Calc-U-1600 app session and a cold boot per test; about 120), split off `CoreTests` by CMake (a file that names `SessionFixture` goes here) so that ctest runs them side by side; `CalcU1600DebuggerSpeed` and `CalcU1600PacingRealClock` run cases of this program | Frequent | 21.2 s |
| `CalcU1600AppSessionTests` | The `TEST`s of the files of `app/tests/` that use `SessionFixture.hpp` (about 60: `app_session_tests.cpp`, `loader_part_tests.cpp`, `machine_session_tests.cpp`, `media_session_tests.cpp`, `opening_tests.cpp`, `serial_links_tests.cpp`, `shell_product_tests.cpp`), split off by their folder so that ctest runs the three programs side by side | Frequent | 11.2 s |
| `calcu_preset_check.CalcU1600Presets` | `calcu --product c1600 check-preset` over every preset of the product (examples, startup presets, test fixtures, `dev/`, the developer documentation), without exclusions | Frequent | 0.02 s |
| `calcu_preset_check.CalcU1600Templates` | The VS Code extension's templates, filled in as the extension fills them (`tests/fill_vscode_templates.cmake`), checked | Frequent | 0.04 s |
| `CalcU1600MachineIncludeCheck` | `tools/check_machine_includes.sh`: no file outside `core/` (app, frontends, tools, dev) includes a machine class, directly or through a core header, or names one | Frequent | 1.1 s |
| `CalcU1600NoOwnRomLookup` | `library/roms/tests/check_no_own_rom_lookup.sh`: no product source outside `core/Machine/SharpRoms.cpp` (with the host-drive ROM), the catalogue and `app/tests/` looks for a ROM file | Frequent | 0.5 s |
| `CalcU1600NoOldTools` | `tools/check_no_old_tools.cmake` (`cmake -P`): the old CLIs, the probes, `build_tool.sh`, the one-time checks `dev/tape-matrix/` and `dev/loader-matrix/`, their `.run` configurations and targets do not exist; no `PC1600_POWER_PROBE` and no `LegacyCalls.hpp` in the sources | Frequent | 0.02 s |
| `CalcU1600NoPlatformSettings` | `library/settings/check_platform_stores.sh` over `frontends/` with `tools/platform_stores_allowed.txt`: no setting in QSettings or `localStorage`; not registered on Windows | Frequent | 0.02 s |
| `CalcU1600ReadTrace` | `tests/read_trace_check.sh`: the harness writes a short PC-1600 trace through the library's trace session, `tools/read_trace.py` reads it; not registered on Windows or without Python | Frequent | 0.3 s |
| `CalcU1600OpenWithTypes` | `tools/check_open_with_types.sh`: the macOS Open With types of the Qt6 app (`Info.plist.in`): the three preset extensions as the product's own, `.bas`, `.bbin`, `.bin` as Alternate, no `.rom` | Frequent | 0.02 s |
| `CalcU1600NoOwnSession` | `tools/check_no_own_session.cmake` (`cmake -P`): no `AppSession`, `MachineSession` or `Pacer` class in `app/`; the Qt6 frontend includes the library's `sessions/AppSession.hpp`, sends no suspend and runs no operation to its end | Frequent | 0.05 s |
| `CalcU1600DapTests` | `tests/dap/dap_tests.cpp`: the DAP server of `app/dap/` through a test transport in a session of `app/tests/SessionFixture.hpp` (JSON and framing; listening, status, a second client; the set-up, Build & Load, held messages, ending, a new machine, the requests, events, own requests); a program of its own, so that `CoreTests` stays within the budget | Frequent | 1.4 s |
| `CalcU1600DapPortCheck` | `tests/dap/dap_port_check.sh`: `calcu … serve --dap 80` ends with exit status 2 and the reason; so does `--dap` outside `serve`; not on Windows | Frequent | 0.03 s |
| `CalcU1600HelpTests` | `tests/help/help_tests.cpp`: `help/settings-keys.txt` matches `settingsDecl()` (`CALCU1600_HELP_KEYS=1` writes it anew), every declared key is in it, and `help/help.yaml`'s `names:` cover the catalogue's models and peripherals with the bundled module definitions; a program of its own | Frequent | 0.01 s |
| `CalcU1600HelpCheck` | `tools/help check calc-u-1600 --quiet-warnings`: the help tool's checks over `library/help/` and `help/`; registered when Python 3 is found | Frequent | 0.2 s |
| `CalcU1600HelpBuild` | `tools/help build calc-u-1600 --quiet-warnings --out <tree>/products/calc-u-1600/help`: the help asset `Calc-U-1600-help.zip`, so the test suite builds it too | Frequent | 0.3 s |
| `CalcU1600QtTests.romsMissing` | `CalcU1600QtTests roms_missing_in_window` with `CALCU1600_QT_ROMS_MISSING=1`: the window over a ROM folder without one file shows the text in place of the faceplate, no bar, no Machine menu; Help and About work; offscreen, Qt trees only | Frequent | 0.8 s |
| `CalcU1600ShotsThroughAttach` | `frontends/qt6/tests/check_shots_through_attach.cmake` (`cmake -P`): `ShotRunner.cpp` names no `controller()`, `pacer()`, `setFrozen`, `runFor`, `pasteText`: the screenshots go through harness attach; Qt trees only | Frequent | 0.01 s |
| | **Frequent, Calc-U-1600** | | 32.6 s wall alone with `-j8`; both products together 30.9 s wall with `-j8` |
| `CalcU1600WebPlaywright` | `frontends/web/run-tests.sh`: Playwright over `frontends/web/tests/*.spec.js`, Chromium (root, sub-path) and WebKit (offline); the servers are `library/web/tools/serve.py`, the offline server and the prerender sandbox `library/web/tests/dist-helpers.js`: the page, presets (bundled and uploaded), files through `open`, typing, the paper of the plotters, tracing, the cold boot menu item, the startup preset setting | Longer | 22.5 s (Homebrew Emscripten 7.0); 31 s in CI (6.0.13) |
| `CalcU1600RomMissingBuild` | `tests/rom_missing_build.sh`: a Web `dist/` built from `roms/` without one file (default `PC1600-P1-B3-old.bin`) shows the lookup's text naming it and no calculator; not registered on Windows; serial | Longer | 3.2 s (the Web build tree already built) |
| `CalcU1600WebNoTestAids` | `tests/web_no_test_aids.sh`: builds a release Web `dist/` (`frontends/web/build.sh --release`, `CALCU_WEB_TEST_AIDS` off: `app/SharpWebPart.cpp` compiled again) into a temporary folder and finds neither `c1600.upload` nor `c1600.display-text` in any of its files (nor `c1600.shell-product` in the engine); CI runs the script on the `dist/` it zips; not registered on Windows; serial (shares `build/web`) | Longer | 7 s with `build/web` built |
| `CalcU1600WasmSmoke` | `tools/build_wasm_check.sh`: core and app layer under node | Longer | 5.1 s without the build; 44 s in CI with the build |
| `CalcU1600ExampleReferences` | `CoreTests examples_match_their_references` with `CALCU1600_REFERENCES=1`: each bundled example loaded through the library's loader and compared with its reference run (display, BASIC and machine-code areas, paper); `Calculat`, `ce150_demo` (paper prefix), biorhythmus' cursor row and DiskWorks' display are excepted; `make_diskworks_media` also compares the module and disk its `saveas:` writes with those of `examples/dwx/` | Longer | 112 s (all 32 compared) |
| `CalcU1600QtTests` | `frontends/qt6/tests/`: 22 `TEST`s on the real window of Calc-U-1600 put together from the library's shell, on the real ROMs (a bundle folder and home of its own) and the real clock, offscreen, the file dialogs, browser and clipboard replaced: the menus, bar, settings and dialogs, the key under the pointer, display hold, an in-process scenario through attach, the control state, loading a program, opening files (a drop, a QFileOpenEvent as the Dock and Finder's Open With send it, a `.bin`, the start argument). Longer because it waits on the real clock | Longer | 24.5 s |
| `CalcU1600Shots.<scenario>` | `CalcU1600Qt --shots <scenario>` into the build tree (`shots-check/`), through harness attach (`run:` is wall time). The help's scenarios of `docs/developer/screenshots/guide/`, one per help chapter with pictures (`01-introduction`, `02-getting-started`, `04-media`, `05-peripherals`, `06-presets`, `07-debugger`, `08-settings`, `09-product-topics`, the longest about 90 s), and the check scenarios of `tests/gui/*.shots.yaml`, one entry per file: the debugger's status line and Disconnect with a client of the process (the app started with `--dap 32177`), a host SHIFT tap as one SHIFT press (offscreen), plotter paper, tracing, a preset, a module template saved, the media picker. The `method: system` shots need the Screen Recording and Automation grants | Longer | 39.6 s for the help's set (2.2–9.2 s each); 1.7 to 3.6 s per check scenario |
| `CalcU1600DapSmoke` | `tools/dap_smoke.py --port 32170 --harness <calcu>`: the ten scenarios against `calcu --product c1600 serve --dap 32170`, no window, each printed as `dap_<scenario>` (the RENUM one on the fixture ROM of `tests/fixtures/dap/pc1500-bus-rom/`, no skip), and `dap_vscode_session` (launch, breakpoints, stop, step, variables, continue, as VS Code sends them); where attach is built (`CALCU_ATTACH`), not on Windows | Longer | about 6 s of CPU, 17 s with the scenarios' pauses |
| `CalcU1600DapSmokeQt` | `tools/dap_smoke.py --port 32169 --app <CalcU1600Qt>`: the same against the Qt app; Qt trees only | Longer | 33.6 s |
| `CalcU1600HarnessTests` | `tests/harness/`: 54 `TEST`s, each on a `calcu --product c1600` run of its own (`HarnessRun.hpp`), the replies written out. `product_commands_tests.cpp` (29): the `c1600` commands: the BASIC pointers and the program area in the INSPECTOR views `pointers` and `dump-basic` per model, `list-basic` (a PC-1500A, a PC-1600 program across a CE-1600M's two banks, MODE 1 refused), every INSPECTOR view per model, `tape record`/`status` with CSAVE on a CE-150, the CE-150's cassette on the PC-1600, the CE-158 echo program (`tests/fixtures/basic/ce158_echo.bas`) and `examples:interfaces/setcom1600.pc1600` for `serial`, `host-drive` with `FILES "S3:"`, `call` on the LH5801, the Z-80 and the LH5803; `harness_entry_tests.cpp` (6): every function of the harness table, `display-text` with the cursor cell on both models, the entry through `SharpAppPart`; `idle_tests.cpp` (19): the cases of `docs/developer/Idle.md` per model (`idle-state`, `wait`, `wait-finished`, BREAK while a program runs, a tape or the serial port awaited). A program of its own so that `CoreTests` stays within the Frequent budget | Longer | 3.6 s; 10.2 s Debug at `-O1` |
| `CalcU1600MachineStateTests` | `tests/machine_state_tests.cpp`: 5 `TEST`s on `calcu --product c1600` runs (`tests/harness/HarnessRun.hpp`): `state-save` while a program runs, 2 s, `state-restore` in the same run and in another; the replies, the sound and the state after the 2 s equal; the PC-1500, the PC-1500A (also with a CE-150, CE-158 and module), the PC-1600 (also with the CE-1600P, its disk and tape, two modules and the host drive; and with the CE-150 and CE-158); the refusals for another set of peripherals. A program of its own | Longer | 12.4 s Debug at `-O1` |
| `CalcU1600MachineDriver` | `tools/machine/`: the pytest driver on `calcu serve` (`machine.py`, `cardimage.py`, `calcu_pytest.py`, `library/harness/python/calcu_machine.py`), its 35 cases in `test_driver.py` and `test_plugin.py` (pytester); pytest and pytest-xdist through `uv run --with` (fetched once, so Longer); registered where uv is | Longer | 1.1 s |
| `CalcU1600TestingExamples` | `docs/testing/example/`: the testing guide's two examples as a test project of their own (`pytest.ini`, `conftest.py`): MODE 1 with nine module cases, RENUM on the PC-1500 and PC-1500A with three ways in and six commands, 45 cases; through `uv run --with` like `CalcU1600MachineDriver`; registered where uv is | Longer | 1.8 s |
| `CalcU1600DebuggerSpeed` | `CalcU1600SessionTests speed_pc1600_full_speed_with_a_breakpoint` with `CALCU1600_SPEED=1`: a PC-1600 for 20 s of emulated time at full speed, with and without a breakpoint, printed | Longer | 0.5 s (0.22 s and 0.20 s measured) |
| `CalcU1600PacingRealClock` | `CalcU1600SessionTests pacing_longrun_real_clock` with `CALCU1600_PACING_LONGRUN=600`: the PC-1500A and the PC-1600 on the library's app session against the real clock | Checkpoint | 20 min (by design) |
| `CalcU1600QtPacingProbe` | `CalcU1600Qt --pacing-probe 60` (an attach client in `CALCU_ATTACH` builds): a PC-1600 keeps one minute in the Qt6 frontend. The Web half: `frontends/web/tests/session.spec.js` with `CALCU1600_WEB_PACING=1` | Checkpoint | 1 min each; Qt6 −8 ms, Web −2 ms over 60 s |
| `CalcU1600HelpPictures` | `tools/make_screenshots.sh`: the help's pictures into `help/images/` through harness attach, one scenario per help chapter; serial | Checkpoint | not measured |

## Not in a group

Tools and one-time checks, not tests that run again:

- The harness `calcu` itself (`library/harness/cli/`): a tool, tested
  through `calcu_test` and the product entries above.
- Calc-U-59: the goldens generator `tools/goldens/` (regenerated when a
  driver changes), `tools/check_parity.sh` (a branch check), Qt `--grab`
  (by hand).
- Calc-U-1600: `dev/hardware-checks/` (programs for real units).
