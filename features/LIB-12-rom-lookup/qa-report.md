# LIB-12: QA report

Results per acceptance criterion and edge case of `spec.md`.

## M-24 (2026-11-15, branch `m24-rom-lookup`): Library and Calc-U-59

Built and run on an M4 Pro, macOS 27.2, Apple clang 21, CMake 4.5, Ninja,
RelWithDebInfo (`build/tests`, Qt off) and with the Qt6 frontend
(`build/all`). Library cases are in `library/roms/tests/`
(`calcu_library_tests`), Calc-U-59 cases in
`products/calc-u-59/tests/rom_lookup_tests.cpp` (`CalcU59Tests`). Frequent:
11 of 11 CTest entries pass (`calcu_library_tests` 204 `TEST`s,
`CalcU59Tests` 202 `TEST`s, 0 failed, 0 skipped); Calc-U-59's part 24.0 s.

### Library

| ID | Result | Test case |
|---|---|---|
| AC-1 | Pass | `ac1_entry_states_bundle_name_encoding_amount_unit_md5` (`full_set_tests.cpp`) |
| AC-2 | Pass | `ac2_file_of_exactly_that_name_is_read`, `ac2_name_differing_in_case_is_missing`, `ac2_subfolders_are_not_searched` (`bundle_tests.cpp`); "no other part of a product looks for a ROM file": CTest `CalcU59NoOwnRomLookup` (`library/roms/tests/check_no_own_rom_lookup.sh`, T7; it found the six old reader uses in the Qt6 and Swift frontends before they were changed) |
| AC-3 | Pass | `ac3_binary_checked_by_byte_size`, `ac3_text_checked_by_reader_amount`, `ac3_product_set_checks_run_after_file_checks`, `ac3_md5_is_not_checked_at_start` (`lookup_tests.cpp`) |
| AC-4 | Pass | `ac4_full_set_has_every_model_variant_and_peripheral_file` (`full_set_tests.cpp`); `fullset_conflicting_entries_for_one_name_are_a_catalogue_error` (support) |
| AC-5 | Pass | `ac5_every_missing_or_bad_file_named_and_no_set`, `ac5_message_says_full_set_needed_and_offers_nothing_else` (`lookup_tests.cpp`); the product side in AC-10 |
| AC-6 | Pass | `ac6_images_for_every_model_variant_peripheral`, `ac6_build_from_set_never_misses_a_rom`, `ac6_no_file_read_after_lookup` (`rom_set_tests.cpp`) |
| AC-7 | Pass (the text); harness clause open | `ac7_harness_text_names_the_files` (`lookup_tests.cpp`); `tools/headless` ends naming the file (`CalcU59HeadlessRomMissing`). The harness (`calcu`) is LIB-11's, M-28: per design D-1 the clause passes there and LIB-12 Library becomes Done then |
| EC-1 | Pass (library half) | `ec1_file_of_two_entries_is_in_the_set_once_with_both` (`full_set_tests.cpp`), `ec1_file_of_two_entries_is_named_once_in_the_message` (`lookup_tests.cpp`); the product case is Calc-U-1600's (M-27) |
| EC-2 | Pass | `ec2_text_with_other_line_endings_checked_alike` (`lookup_tests.cpp`); Calc-U-59 below |
| EC-3 | Pass | Web: `lib12_ec3_rom_file_that_cannot_be_fetched_is_a_failed_start` (`frontends/web/tests/rom-missing.spec.js`, `CalcU59WebPlaywright`, Longer) |

Support: `library/formats/Yaml.hpp` moved from Calc-U-1600's `core/` (D-4),
namespace `calcu::formats`; its 8 cases moved from Calc-U-1600's
`memory_card_tests.cpp` to `library/formats/tests/yaml_tests.cpp`, plus
`test_yaml_format_version_required_and_checked`. Calc-U-1600's Frequent
suite (`CoreTests` 803 `TEST`s, `CalcU1600CheckPresets`) passes with the
moved reader.

### Calc-U-59

| ID | Result | Test case |
|---|---|---|
| AC-1 | Pass | `lib12_ac1_ti_entries_have_the_form` |
| AC-8 | Pass | `lib12_ac8_amounts_per_file_and_per_model`, `lib12_ac8_ti59_first_and_last_word`, `lib12_ac8_wrong_sentinel_is_bad`, `lib12_ac8_module_without_data_is_bad` |
| AC-9 | Pass | `lib12_ac9_full_set_is_the_24_files_with_their_md5` (the spec's table, and the md5 of each fetched file), `lib12_ac9_fetch_script_has_the_same_names_and_md5` (parses `roms/fetch_roms.sh`: 24 files, flat, no module dumps); `roms/fetch_roms.sh` fetched all 24 from a local TI-59-ROM checkout with their md5 checked; `CalcU59XcodeProjectCheck` (the app bundles the 10 dumps and 14 module files, no 2.3.0 path) |
| AC-4 (59) | Pass | `lib12_ac4_every_model_and_module_needs_only_set_files` |
| AC-6 (59) | Pass | `lib12_ac6_model_and_module_changes_read_no_file` (the bundle folder removed after the start; all models and the 14 modules) |
| AC-10 | Pass | `lib12_ac10_missing_file_no_machine_and_files_named`, `lib12_ac10_bad_file_no_machine` (FakePlatform: no controller, `romsMissing` with the text). Web: `lib12_ac10_bad_rom_file_shows_the_text_and_no_calculator`, `lib12_ac10_missing_rom_file_shows_the_text_and_no_calculator` (Playwright). Swift: `LayoutTests/testRomMissingShowsMessage` (XCUITest, iPhone 18 Pro Max simulator, iOS 27.2; DEBUG launch variable `CALCU59_ROM_LEAVE_OUT`), passed. Qt6, by hand (D-8): built without `TMC0583.txt`, the app shows the dialog "Calc-U-59 cannot start" naming `TMC0583.txt` (needed by TI-59, TI-58) and the full set (screenshot, not kept); `main()` then returns 1 (read from the code, not run to the end); with the full set it starts as before (offscreen screenshot). Headless: `CalcU59HeadlessRomMissing` |
| AC-14 (TI) | Pass | `lib12_ac14_module_rom_from_rom_yaml` (5000 bytes, `format-version: 1`, `module-name` = the file's ID), `lib12_ac14_module_images_equal_the_2_3_0_dumps` (md5 of each decoded image equal to 2.3.0's decoding of `TMC05xx.txt`, computed in this step from TI-59-ROM `dumps/modules/` before they left the fetch list), `lib12_ac14_unknown_format_version_is_bad` |
| EC-2 | Pass | `lib12_ec2_crlf_and_cr_dumps_check_alike` (all 24 files with `\r\n` and with `\r`: same words, rows and module images; a TI-58C builds) |
| EC-3 | Pass | as Library |
| Technical Requirements: a build with one ROM file taken away | Pass | `tests/rom_missing_build.sh` (CTest `CalcU59RomMissingBuild`, Longer): `roms/` without `TMC0583.txt`; the build succeeds (D-5); `ti59.py` exits 1 naming the file; a Web `dist/` built from it shows the text naming the file and no calculator (`lib12_build_with_one_rom_file_taken_away_names_it`) |

Golden runs: all 17 of `products/calc-u-59/tests/golden-runs/` pass
unchanged through the lookup (`golden_runs_tests.cpp`; the module runs
`ti59-module-ml`, `ti59-module-le` now take their bytes from the
`.rom.yaml` files). No reference was re-recorded.

Other checks of the step:
- Qt6 frontend (`build/all`, Homebrew Qt): builds; starts and draws the
  calculator (offscreen `--grab`).
- Swift: `build_xcframework.sh` and `xcodebuild … -destination
  platform=macOS build`: BUILD SUCCEEDED; the XCUITests
  `testRomMissingShowsMessage`, `testResetButtonVisible` on the iOS
  simulator: passed.
- Web: `frontends/web/build.sh` (Emscripten, Homebrew `emcc`) builds;
  `run-tests.sh`: 64 passed, 3 skipped, and `dragging moves the press to
  the key under the pointer` fails now and then (below).
- `CalcU59NoOwnRomLookup`: the product's sources outside `app/ProductRoms.*`,
  `core/TICatalogue.cpp` and `frontends/qt6/app/QtResourceBundle.cpp` open no
  ROM file.

### Deviations from the design

- **`SessionController::create`.** The start is one call in the app layer:
  `create(platform, bundle, cuecards, moduleID)` runs `lookUpRoms` and either
  returns the controller or calls `Platform::romsMissing` (A-4) and returns
  null. Qt6 and the Web frontend use it; the Swift frontend looks up first
  through a small bridge class `TIRomSet` (so the view model never makes a
  controller without ROMs) and passes the set to the controller.
- **Qt6**: the lookup runs in `EmulatorController`'s constructor through
  `create`; `main()` ends with exit code 1 when it failed (design: "before
  the SessionController" in `EmulatorController.cpp`, the same order).
- **`FullSet::catalogueErrors`** holds the file names of the conflicting
  entries; `lookUp()` makes their `Problem::Catalogue` rows.
- **`FileProblem`/`Problem`** are declared in `roms/RomReader.hpp` (the
  product's `checkSet` returns them), included by `roms/Lookup.hpp`.
- **The `roms-missing` topic** of LIB-3 is not sent yet: until LIB-3 59
  (M-33/M-34) adapter A-4 (`Platform::romsMissing`) carries it, as §3.5 says.
- **`core_tests.cpp` `rom_missing_file_throws`** (C59-1 AC-5, name kept) now
  checks that the lookup over an empty folder gives no set and names 24
  files; `shared_controller_tests.cpp`
  `controller_rom_failure_keeps_the_running_machine` is removed (the
  behaviour of AC-10's "today" goes); `controller_bad_module_changes_nothing`
  expects no alert (a module ID `cuecards.txt` does not list is not a ROM
  error).
- **Module encodings**: `core/ModuleFile` reads `addressed-hex` (the 14
  files), `hex` and `base64` (`docs/formats/README.md` §1.1).
- **`tools/headless`**: `ti59.py` takes another ROM folder from
  `TI59_ROMS_DIR`; `capi.cpp` has `h_error()` for the lookup's text.
  `frontends/web/build.sh`, `build.js` and `run-tests.sh` take
  `CALCU59_ROMS_DIR`/`CALCU59_WEB_DIST` for `rom_missing_build.sh`.
- **AC-9's md5 table** needed no edit: it already lists the 14 `.rom.yaml`
  files with the values `fetch_roms.sh` checks (decision-log row of
  2026-11-14, `76e9452`; design §9), and P2 tests it as it stands.

### Findings

- **Flaky Web test, not from this step.** `play-calculator.spec.js`
  "dragging moves the press to the key under the pointer" fails about one
  run in six (the display shows `7`, not `8` or `78`). The same test built
  from `main` (`b498051`) shows `7` in 6 of 30 runs too. Not changed here.
- **Xcode derived data**: a first `xcodebuild` in a fresh tree, before
  `CalcU59.xcframework` exists, remembers "no XCFramework found" in its
  derived data even after the framework is built; a fresh `-derivedDataPath`
  builds. Unchanged from before this step.

### Open

- LIB-12 Library: AC-7's harness clause (M-28, D-1). Calc-U-1600 column:
  M-27 (below).

## M-27 (2026-11-15, branch `m27-rom-lookup-1600`): Calc-U-1600

Built and run on an M4 Pro, macOS 27.2, Apple clang 21, CMake 4.5, Ninja,
RelWithDebInfo (`build/tests`, Qt off), the Qt6 frontend (`build/all`,
Homebrew Qt) and the Web client (Homebrew `emcc`). Cases are in
`products/calc-u-1600/tests/rom_lookup_tests.cpp` (`CoreTests`). Frequent:
17 of 17 CTest entries pass; `CoreTests` 864 `TEST`s, 0 failed, 0 skipped,
the nine LIB-12 cases 193 checks. Longer, run by hand: `CalcU1600WebPlaywright`
47 passed, 1 skipped (the `taken_away` case, which only
`rom_missing_build.sh` runs); `CalcU1600RomMissingBuild` passed;
`CalcU1600WasmSmoke` passed.

| ID | Result | Test case |
|---|---|---|
| AC-11 | Pass | `lib12_ac11_binary_dumps_checked_by_byte_size` (a 16383-byte `PC-1500_A01.ROM` is the one bad file; the fetched folder passes) |
| AC-12 | Pass | `lib12_ac12_full_set_is_the_21_files_and_ce502b` (the spec's table copied into the test: names, bytes, md5, binary; the CE-502B a text entry of 16384 bytes; the host-drive ROM not in the set; the md5 of every fetched file), `lib12_ac12_fetch_script_has_the_same_names_and_md5` (parses `roms/fetch_roms.sh`: 22 files, no copy into the Qt6 resources) |
| AC-13 | Pass | `lib12_ac13_missing_old_set_does_not_start` (a `MachineSession` over a folder without `PC1600-P0-B0-old.bin` builds no machine, also no PC-1500A; `romMissing` once with the file, who needs it and the folder; no notice), `lib12_ac13_missing_ce1600p_old_file_does_not_start` (and a missing New file alike), `lib12_ac13_no_rom_read_during_session` (the bundle folder removed after the start; the three PC-1500 revisions, CE-150, CE-158, PC-1600 Old/New, CE-1600P Old and the swap to New all work). CLIs: `CalcU1600CliRomLookup` (`tests/cli_rom_tests.sh`: `pc1600_cli --preset` without `PC1600-P1-B3-old.bin` exits 1 naming it; with the full set it runs). `CalcU1600NoOwnRomLookup` (T7). Web: `lib12_ac13_bad_rom_file_shows_the_text_and_no_calculator` (a truncated CE-1600P Old file), `lib12_ac13_missing_rom_file_shows_the_text_and_no_calculator` (the CE-502B left out of `files.json`), Playwright, Longer. Qt6, by hand (D-8): with the full set the built app runs the screenshot scenario `02-getting-started` offscreen (6 shots, exit 0); with `PC1600-P1-B3-old.bin` removed from the bundle's `Resources/` it stops before any machine in the modal dialog (offscreen: no shot, still waiting after 15 s). The dialog's text is `describe()` (tested above); the dialog itself was not seen on screen |
| AC-14 (CE-502B) | Pass | `lib12_ac14_ce502b_definition_from_the_set` (the definition from the set: `format-version: 1`, `module-name: CE-502B`, a ROM module; its content yields 16384 bytes, so design §9's "not verified" is settled; a definition that does not parse is a bad file; the session's catalogue has it from the set), `lib12_ac14_ce502b_not_in_qt6_resources` (no `ce502b.card.yaml` in the Qt6 resources or `.gitignore`; the card scans pass over the set's file); `ce502b_tests.cpp` and `descriptor_tests.cpp` take it from the set |
| EC-1 | Pass | `lib12_ec1_ce1600p_b5_file_once_in_set_and_message` (each B5 file once in the full set; without the Old one the message names it once, needed by "CE-1600P (variant Old)") |
| Technical Requirements: a build with one ROM file taken away | Pass | `tests/rom_missing_build.sh` (CTest `CalcU1600RomMissingBuild`, Longer): a Web `dist/` built from `roms/` without `PC1600-P1-B3-old.bin` (the build succeeds, D-5) shows the text naming it and no calculator; the CLI side is `CalcU1600CliRomLookup`, Qt6 by hand as in AC-13 |

Golden runs: all 23 of `products/calc-u-1600/tests/golden-runs/` pass
unchanged with the images from the ROM set (`golden_runs_tests.cpp`, through
`sharptest::modelImages` / `peripheralImages` over `testRomSet()`). No
reference was re-recorded.

Other checks of the step: the Qt6 frontend builds and its bundle has
`CE-502B.card.yaml` beside the ROMs and no `ce502b.card.yaml`;
`frontends/web/build.sh` builds; `tools/build_wasm_check.sh` passes.

### Deviations from the design

- **Where the lookup lives.** `sharp::fullRomSet()`, `sharp::lookUpRoms()`
  are in `core/Machine/SharpRoms.{hpp,cpp}`, not `app/ProductRoms`: the
  CLIs and probes link the core only, the probes a copy of their own
  (`calcu_core_probe`), so an `app/` function could not reach them. The
  reader is `core/SharpRomReader` as designed.
- **Who makes the Bundle.** Both frontends' bundles are the bundled
  resources folder, so `MachineSession` makes the `DirectoryBundle` of
  `AppPaths::bundledResourcesDir()` and looks up once in its constructor;
  `romMissing` (A-5, now `romMissing(const RomsMissing&)`) comes at the
  start's model switch, then the session throws as before. The tools make
  `DirectoryBundle("roms")` (`cli::romSet`, `tools/CliCommon.hpp`).
- **Preset loaders** take `PresetRoms` (`core/Preset/PresetRomSet.hpp`):
  the `RomSet` and, beside it, the host-drive ROM, which a preset's
  `host-drive:` needs and which is not in the set (Q-1). The loading
  helpers that were `BundledRoms::` are `presetroms::` in
  `core/Preset/PresetRoms.*`, taking the files from the catalogue's entries.
- **Host-drive ROM** (D-6): read by `sharp::readHostDriveRom()` in
  `core/Machine/SharpRoms.cpp` (the listed exception of
  `CalcU1600NoOwnRomLookup`), from the bundled resources folder in the app
  and from `roms/` or `firmware/pc1600-hostdrive/` in the tools.
- **The CE-502B from the set, not the folder scan.** `scanMemoryCardDirectory`
  passes over a file of the full set (`sharp::isFullSetFile`; a new
  optional `skip` of `scanNamedFiles`), because in the Qt6 bundle the ROMs
  and the card templates share one folder. `ModuleBay` lists the set's ROM
  modules with the folder's cards and attaches the CE-502B from its text
  (`makeSoftwareDefinedCardFromText`); a preset's `slot-1: CE-502B` takes it
  from the set first (`makePresetModuleCard`); Name & Save refuses its name
  as a bundled one, as before.
- **Tests' ROMs.** `tests/TestRoms.hpp` has `testRomSet()` and
  `testPresetRoms()`; the tests that called `BundledRoms::` call
  `presetroms::` with them. A ROM test now needs the full set in `roms/`.
- **The "Missing ROM File" dialog** of 0.8.1 named a stale
  `tools/fetch_roms.sh`; it is "Calc-U-1600 cannot start" with `describe()`.
  No ledger row (D-8: a release cannot reach this state); likewise for the
  removed New-for-Old substitution and its two notices.
- `tests/rom_lookup_tests.cpp` uses Calc-U-59's test MD5
  (`products/calc-u-59/tests/Md5.hpp`) by relative include rather than a copy.

### Open

- None for the Calc-U-1600 column. LIB-12 Library: AC-7's harness clause
  (M-28, D-1).
