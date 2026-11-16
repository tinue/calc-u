# Test groups

Every test of both products and its group (ADR 0031, Briefing §1.10,
§7; done in M-13 of `docs/migration-plan.md`). The C++ tests of both
products use one runner, `library/testing/TestHarness.hpp` (ADR 0038).

## How the groups run

| Group | CTest label | Budget | Registered in a tree | Runs |
|---|---|---|---|---|
| Frequent | `frequent` | ≤ 60 s per product, locally | always (with `CALCU_TESTS`) | on every change, locally and in CI |
| Longer | `longer` | ≤ 10 min | with `-DCALCU_TESTS_LONGER=ON` | locally before merging into `main` and at the end of a part; Playwright and the WASM smoke also in CI |
| Checkpoint | `checkpoint` | none | with `-DCALCU_TESTS_CHECKPOINT=ON` | before a release |

A plain `ctest` in a tree without the two options is the Frequent run, so
the CI jobs and `tools/run_tests.sh` need no filter. A product registers
each entry with `calcu_add_test(<name> <group> COMMAND …)` (the root
`CMakeLists.txt`): the group is its label, a Longer or Checkpoint entry is
registered only with its option, and it runs in the product directory;
the source checks both products run (`<Product>NoOwnRomLookup`,
`<Product>NoPlatformSettings`) come from `calcu_product_source_checks()`
(`library/CMakeLists.txt`). A tree with both options and the Qt apps:

```sh
cmake -S . -B build/groups -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCALCU_TESTS_LONGER=ON -DCALCU_TESTS_CHECKPOINT=ON
cmake --build build/groups
ctest --test-dir build/groups -L longer        # or -L frequent, -L checkpoint
ctest --test-dir build/groups/products/calc-u-1600 -L longer   # one product
```

Prerequisites outside the tree: the products' Playwright entries test a built
`frontends/web/dist/` (`frontends/web/build.sh`); the library's builds its
test product itself (Emscripten, `build/web`); the WASM smoke builds
`build/web` with Emscripten itself; the XCUITest entries need the
simulators of `bin/simulators.yaml` (`bin/simulators setup`); the
Calc-U-1600 screenshot and DAP entries need the Qt app in the tree.
Run the Longer group without `-j`: the products' Playwright entries and
their ROM-missing builds share ports 8059/8060 and `build/web`, and the
XCUITest entries share the simulators (end of C1, 2026-11-15).

A test that outgrows its group's budget moves up a group or is split
(ADR 0031).

Longer, measured locally on 2026-11-14 at the end of Part B: 291.5 s,
within the 10 min; all entries ran (the two Calc-U-1600 web entries since
the `<cstdlib>` fix).

## Library

| CTest entry | What | Group | Measured (M4 Pro, 2026-11-14) |
|---|---|---|---|
| `calcu_library_tests` | 923 `TEST`s (M-42, M-43, M-41, M-40, M-39, M-38, M-37, M-36; 19 of them harness attach, `library/harness/attach/tests/`, with `CALCU_ATTACH`, on by default in a POSIX tree with the tests; real sockets on 127.0.0.1 and a host loop on the real clock, about 3 s; M-33): the library's own tests with the machines built for the tests (`library/machine/tests/TestMachines.*`) and the product declared for the settings tests (`library/settings/tests/TestProduct.hpp`), no product code (LIB-1 AC-24; LIB-1 design §2a); also run alone by the CI job `library` of both product workflows. The pacing tests run on a scripted wall clock (`library/pacing/tests/TestSliceRunner.hpp`), never the host's; so do the session tests (`library/sessions/tests/ScriptedClock.hpp`), with the test frontend and the test product part of `library/sessions/tests/`. LIB-2's host links are tested here with an injected link factory (`peripherals/tests/host_links_tests.cpp`), on every platform; the ROM lookup's tests write their ROM files into a temporary folder (`library/roms/tests/TestRomFiles.hpp`); the harness tests run the product made for them (`library/harness/tests/TestProduct.*`: `test-seg` offering the machine state, `test-dot`) in virtual time, each run in a temporary run folder; LIB-18's tests run `library/startup/tests/TestProductMachines.hpp` (the test product, its settings and a session rig, paced and in virtual time) with the stand-in for LIB-6's loader (`TestPresetLoader.hpp`); LIB-5's typing tests run `library/typing/tests/TypingRig.hpp` (`test-type`, a test-seg with a scripted key readiness, a key log and typing support, `TypingTestMachine.*`; the test profile and map, `TestProfiles.hpp`), paced and in virtual time; LIB-6's preset tests run `library/presets/tests/PresetRig.hpp` (an app session with PeripheralsPart, PresetsPart, StartupPart and TypingPart; test-seg with the program of `TestSegProgram` and the memory `test.memory`) and the rules and keywords built for the tests (`TestPresetRules.hpp`), paced and in virtual time, with presets written into temporary folders and the fixtures of `library/presets/tests/fixtures/` (`bad/`, one file per class of error; `bundle/`; `good/`); LIB-8's paper tests run the harness's test product (which got the test printers TD-20 and TP-4 of the paper test catalogue, `paperTestCatalogue()`, and a PaperPart; M-39) and `library/paper/tests/PaperTestKit.hpp` (a printer driven call by call), with the descriptions of the products' printers and the fixtures of `library/paper/tests/fixtures/` (three `.marks` from the converters of AC-39 and five reference pictures compared byte for byte; `CALCU_PAPER_PICTURES=1` writes them anew); LIB-7's media tests run `library/media/tests/MediaFixture.hpp` (an app session with PeripheralsPart, PresetsPart, StartupPart, ControlPart and MediaPart; the media test catalogue `mediaTestCatalogue()`: the test machines with their media places on, `TestBuilder::media`, plus `test-memory-module` and `test-rom-module`; the kinds of `TestKinds.*`, in `calcu_test_machines`; a save folder and a bundled folder of each test's own, paced and in virtual time; M-41); LIB-13's opening tests run `library/opening/tests/TestOpeners.hpp` (stand-ins for LIB-6 and LIB-7 that record their calls, the file kinds `test.listing` and `test.blob`, on LIB-3's test session `Rig`) and, through the real parts, LIB-7's `MediaFixture.hpp` with an `OpenPart` (`FixtureOptions::moreParts`, a settings file of the test's own for EC-7), paced and in virtual time; upload folders are temporary (M-42) | Frequent | 0.64 s (M-28, 2026-11-15, RelWithDebInfo; most of it the five-hour scripted runs of LIB-17) |
| `calcu_library_pty_tests` | 4 `TEST`s of `library/peripherals/tests/pty_tests.cpp`: LIB-2's host links on real pseudo-terminals in a temporary folder for serial links (AC-18 to AC-20, EC-7); a program of its own (with the test machines), registered only where `CALCU_HAS_PTY` is set (macOS, Linux), so that it can move to Longer without a design change (LIB-2 design §8) | Frequent | 0.8 s (M-23, 2026-11-15) |
| `calcu_cli.<case>` | LIB-11 T13: `calcu_test` (the harness with the test product only) in a process of its own, run by `library/harness/tests/cli/cli_cases.cmake` with `cmake -P`: argv, standard input and output, exit status; eight entries, `ac1_product_by_name_or_short_form_any_case`, `ac2_one_shot_runs_in_order_and_exits_zero`, `ac2_first_refusal_exits_nonzero_with_reason`, `ac3_serve_one_reply_per_line_until_quit`, `ec3_serve_ends_at_end_of_input`, `ec4_unknown_product_lists_products` (M-28), `ac34_check_preset_exit_status` (LIB-6 AC-34, M-38), `ac50_refused_expect_ends_one_shot_nonzero` (LIB-10 AC-50, M-40) | Frequent | 0.1 s, the eight together |
| `calcu_preset_check.LibraryPresetFixtures` | LIB-6 L14 (design D-20): `calcu_add_preset_check()` (`library/presets/PresetChecks.cmake`, run by `RunPresetCheck.cmake` with `cmake -P`): `calcu_test check-preset` over `library/presets/tests/fixtures/good/*` and a template filled in by `fixtures/fill.cmake`; fails when a file fails. The products' checks of AC-43 and AC-51 use the same function from M-46 and M-54 (M-38) | Frequent | 0.05 s |
| `calcu_serve_python` | LIB-11 T15: `library/harness/python/tests/test_calcu_serve.py`, the Python helper for `calcu serve` against `calcu_test` (with `uv run --no-project python -I`, else Python 3; M-28) | Frequent | 0.05 s |
| `LibraryPlatformStoreCheck` | `library/settings/tests/check_platform_stores_test.sh`: the self-test of `library/settings/check_platform_stores.sh`, the source check the products run over their frontends (LIB-9 design T-14); not registered on Windows | Frequent | 0.03 s |
| `calcu_qt6_tests` | 47 `TEST`s: 10 of `library/qt6/tests/paper_panel_tests.cpp` (LIB-8 T13, T14: AC-12 to AC-16, AC-18 to AC-20 on the Qt6 paper panel and `ClipboardService`, an app session with PaperPart and the test printers; M-39), 3 of `media_components_tests.cpp` (LIB-7 L11: AC-8, AC-9, AC-16 on `MediaPicker` and `NameAndSaveDialog`, without a window; M-41), 1 of `run_control_buttons_tests.cpp` (LIB-10 L16: AC-18 on `RunControlButtons`, the rule's table and an app session with the DebuggerPart, a held step's real 250 ms repeats; M-40); 28 of `shell_widgets_tests.cpp` (LIB-14 L7: AC-1 to AC-21, AC-23, AC-24, AC-43, EC-2, EC-4, EC-6 on the shell's widgets, the test window `tests/TestWindow.*` over `TestShellPart` and SettingsPart on a scripted clock; the shortcut and touch cases with Qt Test, skipped without it; M-43); 1 of `shell_widgets_tests.cpp` for LIB-7 AC-29 (`lib7_ac29_qt6_file_menu_without_media_items`: no media items in the Qt6 File menu, LIB-7 OQ-4; M-42); 4 of `attach_host_tests.cpp` (LIB-14 L8, AC-44 support: harness attach in `SessionHost` with LIB-11's test product, real sockets on 127.0.0.1 and the 60 Hz timer, about 1 s; only with `CALCU_ATTACH`; M-43): the Qt6 component set's one test program (`calcu_qt6`, `QtTestMain.cpp`; LIB-10, LIB-7 and LIB-14 add files to it), `QT_QPA_PLATFORM=offscreen`; registered only in a tree with `CALCU_FRONTEND_QT` where Qt6 Widgets is found (not in `build/tests`, which is configured with `-DCALCU_FRONTEND_QT=OFF`) | Frequent | 2.7 s (M-43; 1.8 s of it the paper toast's real wait, about 0.6 s the attach host on the real clock) |
| `calcu_qt6_scenarios` | 9 `TEST`s of `library/qt6/tests/shell_scenarios.cpp` (LIB-14 L10: the scenarios of AC-25 under their IDs, `ac1_the_menus`, `ac4_a_shortcut`, `ac6_the_platforms_places`, `ac8_the_bar`, `ac11_the_settings_dialog`, `ac15_loading`, `ac17_a_notice`, `ac20_help_opens`, `ac24_a_key_under_the_pointer`, on the test window, widgets found by the object names of LIB-14 §3.10; M-43); offscreen; registered with `calcu_qt6_tests` | Frequent | 0.2 s (M-43) |
| `LibraryShellNoProduct` | `library/qt6/tests/check_no_product_code.sh`: no include from `products/` and no model, peripheral or product name in the code (comments aside) of `library/shell`, `library/qt6` and `library/web/components` (LIB-14 L11, AC-22; M-43); not registered on Windows | Frequent | 0.2 s (M-43) |
| `LibraryWebNodeTests` | `node --test library/web/tests/build.test.js library/web/tests/shell_model.test.js`: 4 tests of the Web frontend shell's build functions (`library/web/build/dist.js`: MANIFEST, `files.json`, the service worker stamp, one global in `calc.js`, the CSS scope check; LIB-15 AC-14, AC-15, AC-17; M-29) and 40 of the React set's shell models (`library/web/components/shellModel.js`, the mirror of the C++ model cases under the same names, on `library/shell/tests/inputs.txt` and `hit_cases.txt`; LIB-14 L1 to L6; M-43); registered when `node` is found; also run by the CI job `library` | Frequent | 0.3 s (M-43, 2026-11-15) |
| `LibraryWebPlaywright` | `library/web/tests/run-tests.sh`: builds the product made for the tests (`library/web/tests/product/build.sh`: `calcu_web_test_product` in `build/web`, then `Test-A`, `Test-B`, a second release of `Test-A` and `Test-M` (with LIB-7's MediaPart, M-42) in `build/web/library/web/test-dist/`), then Playwright: `hosting`, `storage`, `embed` in Chromium (root, sub-path) and WebKit (root), `offline` in Chromium and WebKit, `prerender` in Node (LIB-15 AC-1 to AC-18, EC-1 to EC-5; M-29); `paper` (`tests/paper.spec.js`, LIB-8's React paper panel on the test product with TD-20 and TP-4: AC-12 to AC-15, AC-19 to AC-21; 9 cases) in Chromium and WebKit (M-39); `run-control` (`tests/run-control.spec.js`, LIB-10 L12, L16: the React run-control buttons on the rule's table and the test product's DebuggerPart, AC-18; an ended trace as a download, AC-26; 2 cases) in Chromium and WebKit (M-40); `shell` (`tests/shell.spec.js`, LIB-14 L9: the React set's shell on the test product with `shell=1`, TestShellPart and SettingsPart; the AC-25 scenarios `ac1`, `ac4`, `ac6`, `ac8`, `ac11`, `ac15`, `ac16`, `ac17`, `ac19`, `ac20`, `ac22`, `ac24`, `ac43` and LIB-7's `ac29`; 14 cases) in Chromium and WebKit (M-43); `media` (`tests/media.spec.js`, LIB-7 L11: the React media components on `Test-M` with `shell=1`: `ac29` File menu and the persistent file system, `ac8`, `ac9`, `ac16`, `ac31`; 6 cases) in Chromium and WebKit (M-42); 138 cases, 2 skipped (`ec1` runs in the Chromium root project only); not registered on Windows; in CI in the `web` job of both product workflows, with each one's Emscripten (LIB-15 D-8) | Longer | 35 s, plus 10 s for the build with `build/web` built (M-29, Homebrew Emscripten 7.0) |

`calcu_library_tests` by file (`library/<area>/tests/`; every `TEST` is
Frequent):

| File | `TEST`s |
|---|---|
| `harness/attach/tests/attach_tests.cpp` | 17 (C59-5 AC-10–12, AC-14, EC-1–4; M-33) |
| `harness/attach/tests/attach_cli_tests.cpp` | 2 (`calcu attach`, C59-5 AC-10, AC-11; M-33) |
| `machine/tests/attach_tests.cpp` | 19 (LIB-2; `can_replace_answers_as_replace_does`, LIB-18 AC-12, M-35) |
| `machine/tests/build_alone_tests.cpp` | 1 |
| `machine/tests/build_tests.cpp` | 3 |
| `machine/tests/capability_tests.cpp` | 6 |
| `machine/tests/conformance_tests.cpp` | 2 |
| `machine/tests/descriptor_tests.cpp` | 6 |
| `machine/tests/determinism_tests.cpp` | 1 |
| `machine/tests/display_tests.cpp` | 5 |
| `machine/tests/emulated_time_tests.cpp` | 3 |
| `machine/tests/keys_tests.cpp` | 4 |
| `machine/tests/operations_tests.cpp` | 2 |
| `machine/tests/sound_tests.cpp` | 2 |
| `machine/tests/time_tests.cpp` | 5 |
| `settings/tests/ini_document_tests.cpp` | 7 |
| `settings/tests/paths_tests.cpp` | 21 |
| `settings/tests/settings_file_tests.cpp` | 15 |
| `settings/tests/startup_tests.cpp` | 4 |
| `pacing/tests/jitter_tests.cpp` | 4 |
| `pacing/tests/pacer_tests.cpp` | 20 |
| `pacing/tests/pacing_longrun_tests.cpp` | 2 (LIB-17 AC-2, AC-3: five hours on a scripted clock, Frequent) |
| `sessions/tests/command_tests.cpp` | 10 (`deferred_reply_comes_later_and_not_busy`, LIB-5 D-4, M-36) |
| `sessions/tests/host_call_tests.cpp` | 7 (`advance_unpaced_runs_parts_after_call`, LIB-8 A-6, M-39) |
| `sessions/tests/host_service_tests.cpp` | 4 |
| `sessions/tests/library_topics_tests.cpp` | 3 |
| `sessions/tests/mirror_tests.cpp` | 1 |
| `sessions/tests/operation_tests.cpp` | 20 (LIB-18 D-6, M-35; `OperationContext::hostWait`, LIB-6 D-17, M-38; `OperationResult::result`, `SessionContext::startOperation`, LIB-13 L2, M-42) |
| `sessions/tests/sound_tests.cpp` | 5 |
| `sessions/tests/speed_tests.cpp` | 6 |
| `sessions/tests/suspend_tests.cpp` | 5 |
| `sessions/tests/topic_tests.cpp` | 10 (`before_flush_runs_at_every_flush`, LIB-4 L3, M-37) |
| `sessions/tests/value_tests.cpp` | 1 |
| `peripherals/tests/controls_tests.cpp` | 6 |
| `peripherals/tests/host_links_tests.cpp` | 2 |
| `peripherals/tests/real_time_clock_tests.cpp` | 4 |
| `peripherals/tests/serial_port_tests.cpp` | 4 |
| `peripherals/tests/time_tests.cpp` | 2 |
| `roms/tests/bundle_tests.cpp` | 3 (LIB-12 AC-2) |
| `roms/tests/full_set_tests.cpp` | 4 (LIB-12 AC-1, AC-4, EC-1) |
| `roms/tests/lookup_tests.cpp` | 9 (LIB-12 AC-3, AC-5, AC-7, EC-1, EC-2) |
| `roms/tests/rom_set_tests.cpp` | 3 (LIB-12 AC-6) |
| `formats/tests/yaml_tests.cpp` | 11 (the YAML reader, moved from Calc-U-1600 in M-24; no AC; the preset options and `parseNumber`, LIB-6 D-4, M-38) |
| `formats/tests/formats_tests.cpp` | 3 (LIB-11 AC-12, AC-22, EC-2; M-28) |
| `formats/tests/png_tests.cpp` | 1 (the PNG writer, LIB-8 T5; M-39) |
| `harness/tests/protocol_tests.cpp` | 8 (LIB-11 AC-2, AC-4, AC-5, AC-13, AC-22; M-28) |
| `harness/tests/commands_tests.cpp` | 20 (LIB-11 AC-5, AC-7, AC-9, AC-11, EC-2, EC-5, EC-7; M-28; LIB-4 AC-9, M-37; `key`, `type`: LIB-5 AC-15–17, D-5, M-36; `open <path>`, LIB-13 AC-9, M-42) |
| `harness/tests/display_text_tests.cpp` | 10 (LIB-11 AC-9, AC-10, AC-23; M-28) |
| `harness/tests/run_tests.cpp` | 19 (LIB-11 AC-2, AC-6, AC-8, AC-9, AC-21, AC-24, EC-1, EC-3, EC-6; M-28, M-35) |
| `harness/tests/startup_commands_tests.cpp` | 4 (LIB-18 AC-15; M-35) |
| `harness/tests/preset_commands_tests.cpp` | 11 (LIB-6 AC-34; M-38) |
| `harness/tests/paper_commands_tests.cpp` | 9 (LIB-8 AC-23, EC-2, EC-6, EC-8; M-39) |
| `harness/tests/media_commands_tests.cpp` | 5 (LIB-7 AC-26, AC-28; LIB-11 AC-8, AC-22 with media; M-41) |
| `presets/tests/reader_tests.cpp` | 13 (LIB-6 AC-1, EC-1, EC-8; M-38) |
| `presets/tests/header_tests.cpp` | 12 (LIB-6 AC-2 to AC-5, AC-8, AC-9; M-38) |
| `presets/tests/check_tests.cpp` | 9 (LIB-6 AC-6, AC-7, AC-30, AC-62, EC-5; the bad fixtures; M-38) |
| `presets/tests/build_tests.cpp` | 6 (LIB-6 AC-10, AC-52, AC-55, EC-2, EC-10; M-38) |
| `presets/tests/operation_tests.cpp` | 8 (LIB-6 AC-12, AC-13, AC-16, AC-61, EC-3, EC-4, EC-6; M-38) |
| `presets/tests/waits_tests.cpp` | 12 (LIB-6 AC-14, AC-15, AC-17, AC-56 to AC-60; M-38) |
| `presets/tests/log_tests.cpp` | 2 (LIB-6 AC-18; M-38) |
| `presets/tests/pause_tests.cpp` | 5 (LIB-6 AC-19; M-38) |
| `presets/tests/steps_tests.cpp` | 20 (LIB-6 AC-20 to AC-28, AC-63, AC-66, EC-7; M-38) |
| `presets/tests/place_tests.cpp` | 4 (LIB-6 AC-29, AC-53, AC-65; M-38) |
| `presets/tests/determinism_tests.cpp` | 2 (LIB-6 AC-11, AC-64; M-38) |
| `paper/tests/paper_kind_tests.cpp` | 3 (LIB-8 AC-1, the test printers; M-39) |
| `paper/tests/paper_tests.cpp` | 5 (LIB-8 AC-2 to AC-5, EC-1; M-39) |
| `paper/tests/replies_tests.cpp` | 5 (LIB-8 AC-23, AC-39, EC-2; M-39) |
| `paper/tests/picture_tests.cpp` | 5 (LIB-8 AC-16, AC-17, the reference pictures; M-39) |
| `paper/tests/paper_part_tests.cpp` | 16 (LIB-8 AC-6 to AC-9, EC-2 to EC-4, EC-7; M-39) |
| `paper/tests/copy_cut_tests.cpp` | 5 (LIB-8 AC-16, AC-18 to AC-20, EC-5; M-39) |
| `paper/tests/paper_step_tests.cpp` | 4 (LIB-8 AC-22; M-39) |
| `media/tests/names_tests.cpp` | 8 (LIB-7 AC-2 to AC-4; M-41) |
| `media/tests/shelf_tests.cpp` | 10 (LIB-7 AC-1, AC-5, AC-46, EC-3; M-41) |
| `media/tests/place_tests.cpp` | 5 (LIB-7 AC-6; LIB-1 AC-40 with D-1, `checkMediaPlaces`; M-41) |
| `media/tests/topic_tests.cpp` | 4 (LIB-7 AC-7; M-41) |
| `media/tests/picker_tests.cpp` | 9 (LIB-7 AC-8 to AC-10; M-41) |
| `media/tests/autosave_tests.cpp` | 17 (LIB-7 AC-11 to AC-15, EC-1, EC-2, EC-8; M-41) |
| `media/tests/save_tests.cpp` | 13 (LIB-7 AC-16 to AC-20, EC-7, EC-9; M-41) |
| `media/tests/preset_media_tests.cpp` | 20 (LIB-7 AC-21 to AC-27, EC-5, EC-6; LIB-6 AC-3 by `file:` and the bundle check of bundled media; LIB-18 AC-2, AC-7 media clauses; M-41) |
| `media/tests/saved_media_tests.cpp` | 9 (LIB-7 AC-1, AC-30, AC-31, EC-4; M-41) |
| `opening/tests/classify_tests.cpp` | 15 (LIB-13 AC-1, AC-2, AC-10, EC-1, EC-5; M-42) |
| `opening/tests/open_tests.cpp` | 25 (LIB-13 AC-3, AC-4, AC-7, AC-8, EC-2, EC-3, EC-4, EC-6; the request's Value form; M-42) |
| `opening/tests/ways_tests.cpp` | 4 (LIB-13 AC-5, AC-6; M-42) |
| `opening/tests/real_parts_tests.cpp` | 3 (LIB-13 AC-1, AC-4 through LIB-6's and LIB-7's parts, EC-7; M-42) |
| `shell/tests/menu_model_tests.cpp` | 13 (LIB-14 AC-1 to AC-7, AC-19, EC-4, LIB-7 AC-29's menu items; M-43) |
| `shell/tests/bar_model_tests.cpp` | 10 (LIB-14 AC-8 to AC-10, AC-23, AC-42, EC-5; M-43) |
| `shell/tests/settings_dialog_tests.cpp` | 8 (LIB-14 AC-11 to AC-14, EC-3, D-19; M-43) |
| `shell/tests/loading_notice_tests.cpp` | 7 (LIB-14 AC-15, AC-17, EC-1, EC-2, EC-7, D-7, N4; M-43) |
| `shell/tests/hit_map_tests.cpp` | 4 (LIB-14 AC-24, AC-43, EC-8; M-43) |
| `shell/tests/help_about_tests.cpp` | 2 (LIB-14 AC-20, AC-21; M-43) |
| `shell/tests/inputs_fixture_tests.cpp` | 1 (the JS mirror's inputs from the payloads; M-43) |
| `presets/tests/list_tests.cpp` | 6 (LIB-6 AC-31 to AC-33, EC-9; M-38) |
| `presets/tests/startup_tests.cpp` | 3 (LIB-18 AC-4, EC-1, EC-6 with the real loader; M-38) |
| `presets/tests/typing_tests.cpp` | 3 (LIB-5 AC-14, AC-15, AC-17 in a preset; M-38) |
| `startup/tests/build_tests.cpp` | 11 (LIB-18 AC-1, AC-2, EC-4; M-35) |
| `startup/tests/start_tests.cpp` | 14 (LIB-18 AC-3, AC-4, AC-5, EC-1, EC-2, EC-6; M-35) |
| `startup/tests/switch_tests.cpp` | 10 (LIB-18 AC-6 to AC-9; M-35) |
| `startup/tests/change_tests.cpp` | 10 (LIB-18 AC-10, AC-11, AC-12, EC-5; M-35) |
| `startup/tests/reset_tests.cpp` | 6 (LIB-18 AC-13, AC-14, EC-3, EC-7; M-35) |
| `startup/tests/running_tests.cpp` | 4 (LIB-18 AC-16; M-35) |
| `control/tests/control_state_tests.cpp` | 14 (LIB-4 AC-1, AC-5, EC-1, EC-3, EC-4; M-37) |
| `control/tests/availability_tests.cpp` | 11 (LIB-4 AC-2, AC-3, AC-7, EC-5; M-37) |
| `control/tests/control_part_tests.cpp` | 4 (LIB-4 AC-4, AC-6, AC-8; M-37) |
| `typing/tests/typing_machine_tests.cpp` | 2 (L1, `test-type`; M-36) |
| `typing/tests/holds_tests.cpp` | 3 (LIB-5 AC-1; M-36) |
| `typing/tests/live_tests.cpp` | 12 (LIB-5 AC-2, AC-4, AC-5, AC-9, AC-10, EC-1, EC-6; M-36) |
| `typing/tests/queue_tests.cpp` | 4 (LIB-5 AC-3, AC-11, EC-3; M-36) |
| `typing/tests/determinism_tests.cpp` | 1 (LIB-5 AC-6; M-36) |
| `typing/tests/map_tests.cpp` | 3 (LIB-5 AC-7, AC-8, EC-2; M-36) |
| `typing/tests/reset_tests.cpp` | 1 (LIB-5 EC-5; M-36) |
| `typing/tests/text_tests.cpp` | 5 (LIB-5 AC-11–13, AC-16, EC-4, D-14; M-36) |
| `typing/tests/part_tests.cpp` | 7 (LIB-5 AC-15, AC-16, AC-18, AC-19, AC-38, AC-39; M-36) |
| `typing/tests/operation_tests.cpp` | 3 (LIB-5 AC-14, AC-17, §3.6; M-36) |
| `typing/tests/pause_tests.cpp` | 3 (LIB-5 AC-20, library part; M-36) |
| `machine/tests/debug_target_tests.cpp` | 12 (LIB-10 L1, L2: AC-1 to AC-4, AC-48, AC-49, EC-9, `checkDebugging`; M-40) |
| `debugger/tests/run_control_tests.cpp` | 33 (LIB-10 L3, L4, L5, L9, L10: AC-5 to AC-8, AC-13 to AC-17, AC-45, EC-1, EC-2, EC-5 to EC-7; M-40) |
| `debugger/tests/start_tests.cpp` | 5 (LIB-10 L6: AC-9, AC-46; M-40) |
| `debugger/tests/breakpoint_tests.cpp` | 12 (LIB-10 L7, L8: AC-10 to AC-12, AC-47, EC-3, EC-4; M-40) |
| `debugger/tests/trace_tests.cpp` | 11 (LIB-10 L11, L12: AC-22 to AC-26, EC-8; M-40) |
| `debugger/tests/steps_tests.cpp` | 7 (LIB-10 L13: AC-19; LIB-6 AC-19's debugger steps, AC-63's `debug-panel:`; M-40) |
| `debugger/tests/button_rule_tests.cpp` | 2 (LIB-10 L16: AC-18, the rule's table; M-40) |
| `harness/tests/debug_commands_tests.cpp` | 16 (LIB-10 L14, L15: AC-20, AC-21, AC-49, AC-50, AC-51, EC-9; M-40) |

## Calc-U-59

| CTest entry | What | Group | Measured (M4 Pro, 2026-11-14) |
|---|---|---|---|
| `CalcU59Tests` | 283 `TEST`s (M-39: `tests/paper_replies_tests.cpp`, LIB-8 AC-39; M-36; `tests/typing_tests.cpp`: LIB-5 AC-20–27, T59-0): unit and ROM-driven integration tests of the core and the app layer, the machine interface (C59-1, LIB-1 59), the ROM lookup (LIB-12 59), the PC-100C and the module slot (C59-6, LIB-2 59), the settings file (LIB-9 59), the golden replays through it (table below), the harness product entry (LIB-11, M-28; its model switch through LIB-18, `harness_switches_to_a_ti58`, M-35), Calc-U-59 on the library's app session and the session thread (LIB-3 59, LIB-17 59, C59-5; M-33), the product's flows on it and the Web part (M-34) | Frequent | 13 s (M-33); 7 s (M-34, RelWithDebInfo) |
| `CalcU59TraceReader` | `tools/read_trace.py` reads the trace `CalcU59Tests` wrote | Frequent | 16.4 s (Python 3.15; 0.3 s in CI) |
| `CalcU59NoOwnSession` | `tests/check_no_own_session.sh`: the Qt6, Swift and (M-34) Web frontends use no `EmulatorSession`, `EmulationRunner`, `TISessionController` or own `SessionController`; no `std::thread` in `app/` but `SessionThread`; the Swift bridge includes no `SessionController.hpp` and calls no tick; Qt6 sends no suspend or continue (LIB-3 AC-28, AC-31; C59-5 B2; M-33) | Frequent | 0.03 s |
| `CalcU59CoreSeam` | `tests/check_core_seam.sh`: no core-internal header outside `core/` and `tests/`, no machine call in the adapters (C59-1 AC-17; M-19) | Frequent | 0.02 s |
| `CalcU59NoOwnRomLookup` | `library/roms/tests/check_no_own_rom_lookup.sh`: no product source outside `app/ProductRoms.*`, the catalogue and the Qt6 Bundle looks for a ROM file (LIB-12 AC-2, design T7; M-24) | Frequent | 1.2 s |
| `CalcU59NoPlatformSettings` | `library/settings/check_platform_stores.sh` over `frontends/` with `tools/platform_stores_allowed.txt`: no setting in UserDefaults, `@AppStorage`, QSettings or `localStorage`; allowed: the trace folder's bookmark (AC-34) and the Playwright check that 2.3.0's entries are not read (LIB-9 AC-21, design T-24; M-32; the page's own entries went in M-34); not registered on Windows | Frequent | 0.02 s |
| `CalcU59HeadlessSmoke` | `tools/headless/ti59.py 5 + 7 =` against the CMake-built `ti59headless` shows 12 (C59-1 T14; M-19; with Python 3) | Frequent | 0.04 s |
| `CalcU59HeadlessRomMissing` | `tests/headless_rom_tests.py`: `ti59.py` on a ROM folder without one file ends naming it, on the full set it starts (LIB-12 AC-7, AC-10; M-24; with Python 3) | Frequent | 0.15 s |
| `CalcU59XcodeProjectCheck` | `frontends/swift/check_xcode_project.sh`: Xcode compiles only Swift and the bridge and links `CalcU59.xcframework` (C59-5, ADR 0013; M-15); the app bundles the 24 ROM files of the full set (LIB-12; M-24); only Debug takes incoming connections, with `Calc-U-59-Debug.entitlements` (C59-5 AC-10, AC-13; M-33); the XCFramework script phase declares its output (M-33) | Frequent | 0.02 s |
| `CalcU59NoAttachSources` | `frontends/swift/check_no_attach.sh --sources`: attach only for the Debug `macos` and `ios-simulator` slices in `build_xcframework.sh`, the bridge's and the Swift call sites under the Debug and platform guard (C59-5 AC-13, B1; M-33). The `--binary` half runs as a build phase of every Release and device build of the app | Frequent | 0.02 s |
| | **Frequent, Calc-U-59** | | **25.9 s** (M-32) |
| `CalcU59WebPlaywright` | `frontends/web/run-tests.sh`: Playwright, Chromium (root, sub-path) and WebKit (offline); `rom-missing.spec.js` (LIB-12 AC-10, EC-3; M-24); since M-34 on the library's shell, with `session.spec.js` (LIB-15 AC-19, AC-22, EC-4, LIB-3 AC-31) and `storage.spec.js` (LIB-15 AC-20, AC-21, LIB-9 AC-23); `play-calculator.spec.js` host keys by the carried letter, two keys, a short click held 100 ms (LIB-5 AC-21–24; M-36) | Longer | 78 s (M-24); 78 s, 85 passed, 5 skipped (M-34); 91 passed, 5 skipped (M-36) |
| `CalcU59ReferenceRuns` | `CalcU59Tests reference_runs` with `CALCU59_REFERENCE_COMPARE=1`: the 57 examples on the app session, each compared with its 2.3.0 recording of M-9 (`tests/reference/v2.3.0-examples/`); a difference is a finding (M-34) unless the test's table of accepted differences names it with its reason (M-36: the driver's wall time on the "recorded after N s" line; `Elapsed:` lines, row 59-23 and M-33; `historic/life.ti59`'s register at the 30-minute cut, since M-33); the references are not re-recorded (user, 2026-11-15) | Longer | 9 s |
| `CalcU59RomMissingBuild` | `tests/rom_missing_build.sh`: roms/ without one file (default `TMC0583.txt`); the headless library and a Web `dist/` built from it start without a calculator and name the file (LIB-12, Technical Requirements; M-24); not registered on Windows | Longer | 3 s (M-24; the Web build tree already built) |
| `CalcU59UIRegression` | XCUITest plan `UIRegressionTests` on the `iphone` simulator; since M-33 with `AttachTests` (harness attach on port 59160: C59-5 S3, S4 Home and back, S5 full speed by a press on the display); `PacingTests` and `ScreenshotTests` skipped | Longer | 178 s, 18 tests (iPhone 18 Pro Max, iOS 27.2; M-33) |
| `CalcU59SwiftUnitTests` | `xcodebuild test -testPlan UnitTests` on the `iphone` simulator: `Calc-U-59Tests`, hosted by the app with `--UITesting`, a second session through the real bridge (C59-5 S1 `MessageOrderTests`, S2 `HostServicesTests`, `SuspendTests` AC-8, `FrameGapTests` AC-5/D-9; M-33) | Longer | 14 s of tests, about 40 s with the build (M-33) |
| `CalcU59NoAttachInRelease` | `frontends/swift/check_no_attach.sh`: builds the Release Mac app (signed) and the Debug iOS device app (unsigned) into `build/xcode-no-attach`; no attach in them, in the Release XCFramework slices and the Debug `ios` slice; the Release app's entitlements (C59-5 AC-13, B1; M-33) | Longer | 25 s with the XCFramework trees built, minutes from nothing (M-33) |
| `CalcU59PacingRealClock` | `lib17_ac18_pacing_longrun_real_clock_ti` (the library's pacer, M-33; 2.3.0's "before" run went with its batches in M-34) with `CALCU59_PACING_LONGRUN=600` | Checkpoint | 10 min (by design) |
| `CalcU59WebPacing` | `frontends/web/run-tests.sh tests/pacing.spec.js --project root` with `CALCU59_WEB_PACING=1`: a TI-59 keeps one minute in the Web frontend (LIB-17 AC-19, P59-5; M-33); since M-34 the session's `ticks` request on the library's pacer: wall 60.009 s, emulated 59.994 s | Checkpoint | 62 s |
| `CalcU59QtPacingProbe` | the Qt6 app (`CALCU_FRONTEND_QT`) with `--pacing-probe 60`, offscreen, a temporary `HOME` in the tree (LIB-17 AC-19; M-33) | Checkpoint | 61 s |
| `CalcU59PacingIOS` | XCUITest plan `PacingCheckpoint` on the `iphone` simulator: `PacingTests.testTI59KeepsOneMinute`, `ticks` over attach on port 59161 (LIB-17 AC-19; M-33) | Checkpoint | 67 s |
| `CalcU59MacAttach` | `tests/mac_attach_check.sh <calcu>`: the Debug Mac app through Launch Services on port 59171; `calcu attach`; one minute in front (LIB-17 AC-19), 30 s covered by a grey window over every screen (`tests/mac_window.swift`; C59-5 AC-9), hidden and shown (AC-8, D-10) (C59-5 S6; M-33) | Checkpoint | about 2 min with the app built |
| `CalcU59SwiftUnitTestsMac` | `xcodebuild test -testPlan UnitTests` on macOS (C59-5 S2's macOS services; M-33) | Checkpoint | 14 s of tests |
| `CalcU59AppStoreScreenshots` | `bin/run-screenshots`: the App Store set (the iPhone Duo is opened flat by hand) | Checkpoint | not measured |
| `CalcU59UITestsMac` | XCUITest plan `UIRegressionTests` on macOS | Checkpoint | not measured (needs a signed runner and the Accessibility permission, `docs/inventory/calc-u-59.md`) |

`CalcU59Tests` by file (`products/calc-u-59/tests/`; every `TEST` is
Frequent except where the table says otherwise):

| File | `TEST`s |
|---|---|
| `core_tests.cpp` | 17 (C59-1 AC-5; C59-6 AC-4, AC-5, M-25) |
| `descriptor_tests.cpp` | 9 (LIB-1 AC-26–33; M-19, M-25) |
| `harness_tests.cpp` | 1 (`harness_drives_a_ti59`: the exit check of M-28, LIB-11 T14; `SKIP` without the ROMs) |
| `golden_runs_tests.cpp` | 1 (the 18 golden replays through the machine interface and, since M-25, the places, C59-1 AC-4, C59-6 AC-3, AC-7, AC-9, AC-10; `CALCU59_GOLDEN_RECORD=1` records a missing reference only) |
| `machine_tests.cpp` | 26 (C59-1 AC-1–3, 6–16, 18, 19, EC-1–4; M-19) |
| `peripherals_tests.cpp` | 20 (C59-6 AC-1, 2, 4–6, 8–11, EC-1–3; LIB-2 AC-24–27; M-25) |
| `pacing_longrun_after_tests.cpp` | 2, "after" (LIB-17 AC-18; M-33): `lib17_ac18_pacing_longrun_scripted_ti` Frequent; `lib17_ac18_pacing_longrun_real_clock_ti` Checkpoint |
| `pacing_tests.cpp` | 7 (LIB-17 AC-15–17; M-33) |
| `session_tests.cpp` | 10 (LIB-3 AC-29, AC-30, AC-32, AC-33; C59-5 AC-8; M-33) |
| `session_thread_tests.cpp` | 10 (C59-5 AC-4–6, AC-8, H1–H3; real clock, about 4 s; M-33) |
| `paper_replies_tests.cpp` | 1 (`lib8_ac39_ti_printouts_turned_into_paper_replies`: the printouts of the M-9 references turned into `paper text` and `paper marks` replies and compared with the committed `.paper-text`, `.paper-marks`; `CALCU59_PAPER_REPLIES=1` writes them; LIB-8 AC-39, M-39) |
| `reference_runs_tests.cpp` | 1, Longer (`CalcU59ReferenceRuns`): returns at once without `CALCU59_REFERENCE_COMPARE` or `CALCU59_REFERENCE_RECORD` (the latter writes to `build/reference-runs/`, never into the references; M-34) |
| `rom_lookup_tests.cpp` | 15 (LIB-12 AC-1, AC-4, AC-6, AC-8–10, AC-14, EC-2; M-24) |
| `part_flows_tests.cpp` | 47: the product's flows on the app session (`SessionRig.hpp`, `PartRig`), ported in M-34 from 2.3.0's controller tests (C59-6 AC-1, AC-6, AC-11: 4 of them, M-25); the runner's three pacing tests went with `CooperativePacing` |
| `shared_keys_tests.cpp` | 6: KEYSTROKES playback (`TIKeystrokePlayback`) over the typing engine (M-36; the key cases moved to `typing_tests.cpp`) |
| `typing_tests.cpp` | 22 (LIB-5 AC-20–27, T59-0, the printer-timing exit check; M-36) |
| `web_part_tests.cpp` | 5 (LIB-15 AC-16, AC-20, AC-22, EC-4; LIB-9 AC-23; M-34) |
| `shared_debug_tests.cpp` | 14 |
| `shared_formats_tests.cpp` | 20 (`settings_resolve` went with `SettingsKeys.hpp`, M-32) |
| `shared_preset_stages_tests.cpp` | 7 |
| `shared_settings_tests.cpp` | 7 (LIB-9 AC-19–22, AC-24, AC-25, AC-35; M-32) |
| `shared_runtime_tests.cpp` | 15 (`runner_keeps_real_time` went with the threaded runner, M-33; the two batch-rule tests with `Pacing.hpp`, M-34; the session tests host the `EmulatorSession` on a test clock) |
| `shared_state_applier_tests.cpp` | 8 |
| `shared_tables_tests.cpp` | 7 |
| `shared_trace_tests.cpp` | 2 |

## Calc-U-1600

| CTest entry | What | Group | Measured (M4 Pro, 2026-11-14) |
|---|---|---|---|
| `CoreTests` | about 1000 `TEST`s (M-46: LIB-6 1600 `sharp_presets_tests.cpp`, C1600-4 `preset_content_tests.cpp`, LIB-18 1600 `machine_control_tests.cpp`, the `lib6_` cases of `harness_tests.cpp`; `preset_tests.cpp`, `pc1600_preset_tests.cpp`, `presetloader_trace_tests.cpp`, `app/tests/shared_preset_catalog_tests.cpp` gone; M-44: C1600-2 `program_files_tests.cpp`, `basic_typing_tests.cpp`, `loader_memory_tests.cpp`, `app/tests/loader_part_tests.cpp`, the C1600-2 cases of the loader, machine-code-file and harness tests; M-45: LIB-8 1600 `plotter_paper_tests.cpp`, `paper_session_tests.cpp`, the `lib8_ac37_` cases of `ce150_tests.cpp`, `ce1600p_tests.cpp`; LIB-10 1600 `debugger_tests.cpp`, the `lib10_ac36`/`ac37`/`ac44`/`ac52` cases of `debug_target_tests.cpp`, `run_control_tests.cpp`; C1600-3 `dap_layer_tests.cpp`; the harness's paper and debugger cases; M-39: `tests/paper_replies_tests.cpp`, LIB-8 AC-39; M-36; `tests/typing_tests.cpp`: LIB-5 AC-14, AC-28–31, AC-33–37, AC-40, AC-41; the PC-1500 key queue's tests gone): unit and integration tests of the core and the app layer (real ROMs, presets, loaders, job equivalence), the machine interface (C1600-1, LIB-1 1600, LIB-2 1600), the settings file (LIB-9 1600), the ROM lookup (LIB-12 1600), the golden replays through it (table below), the harness product entry (LIB-11, M-28; the PC-1600 and its variant through LIB-18, `harness_drives_a_pc1600`, `harness_variant_switch_old`, M-35) | Frequent | 39.0 s (M-30/M-28/M-27, 2026-11-15) |
| `calcu_preset_check.CalcU1600Presets` | LIB-6 AC-51, C1600-4 AC-15 (M-46): `calcu --product 1600 check-preset` over every preset of the product (examples, startup presets, test fixtures, `dev/`, the developer documentation); left out by name until M-47 (LIB-7): `DiskWorks`, `make_diskworks_media`, `pc1500-maxed-out`, `default-pc1600`, `startup-pc1600`, `pc1600-modules` | Frequent | 0.02 s |
| `calcu_preset_check.CalcU1600Templates` | LIB-6 AC-51, C1600-3 AC-19 (M-46): the VS Code extension's templates, filled in as the extension fills them (`tests/fill_vscode_templates.cmake`), checked | Frequent | 0.04 s |
| `CalcU1600MachineIncludeCheck` | `tools/check_machine_includes.sh`: no file outside `core/` (app, frontends, tools, dev) includes a machine class, directly or through a core header, or names one (C1600-1, Technical Requirements; design T13; M-26) | Frequent | 1.1 s |
| `CalcU1600NoOwnRomLookup` | `library/roms/tests/check_no_own_rom_lookup.sh`: no product source outside `core/Machine/SharpRoms.cpp` (with the host-drive ROM, D-6), the catalogue and `app/tests/` looks for a ROM file (LIB-12 AC-2, design T7; M-27) | Frequent | 0.5 s |
| `CalcU1600CliRomLookup` | `tests/cli_rom_tests.sh`: `pc1600_cli --preset` with a `roms/` without one file exits 1 naming it, with the full set it runs (LIB-12 AC-13, design Q2; M-27) | Frequent | 0.2 s |
| `CalcU1600NoPlatformSettings` | `library/settings/check_platform_stores.sh` over `frontends/` with `tools/platform_stores_allowed.txt`: no setting in QSettings or `localStorage` (LIB-9 AC-31, design T-19; M-30); not registered on Windows | Frequent | 0.02 s |
| `CalcU1600ReadTrace` | `tests/read_trace_check.sh`: the harness writes a short PC-1600 trace through the library's trace session, `tools/read_trace.py` reads it (LIB-10 AC-42, design D16-8; M-45); not registered on Windows or without Python | Frequent | 0.3 s |
| `CalcU1600NoOwnSession` | `tools/check_no_own_session.cmake` (`cmake -P`): no `AppSession`, `MachineSession` or `Pacer` class in `app/`, the Qt6 frontend includes the library's `sessions/AppSession.hpp`, sends no suspend and runs no operation to its end (LIB-3 AC-34, AC-36; design P16-1, P16-3; M-31) | Frequent | 0.05 s |
| | **Frequent, Calc-U-1600** | | **40.4 s** (M-30); `CoreTests` alone 47 s in M-45, 51 s in M-46 (Apple M4 Pro, RelWithDebInfo) |
| `CalcU1600WebPlaywright` | `frontends/web/run-tests.sh`: Playwright, Chromium (root, sub-path) and WebKit (offline); M-46: `lib6_ac49`, `lib6_ac54` (bundled and uploaded), `lib10_ac40 trace step fails on the web`, `lib18_ac32 cold boot menu item`, the startup preset setting; `typing.spec.js` (LIB-5 AC-29, AC-33, AC-36; M-36); `play.spec.js` `lib8_ac36 …`, `lib8_ac38 …` (was `a plotter preset draws on the paper`), `lib10_ac40 …`, `lib10_ac41 …` (M-45) | Longer | 55 passed, 2 skipped (M-36); 22.5 s (Homebrew Emscripten 7.0, after `<cstdlib>` was added to `core/Debug/BreakpointTable.cpp`); 31 s in CI (6.0.13) |
| `CalcU1600RomMissingBuild` | `tests/rom_missing_build.sh`: a Web `dist/` built from `roms/` without one file (default `PC1600-P1-B3-old.bin`) shows the lookup's text naming it and no calculator (LIB-12, Technical Requirements, design Q5; M-27); not registered on Windows | Longer | 3.2 s (M-27; the Web build tree already built) |
| `CalcU1600WasmSmoke` | `tools/build_wasm_check.sh`: core and app layer under node | Longer | 5.1 s locally without the build; 44 s in CI with the build |
| `CalcU1600ExampleReferences` | C1600-4 AC-16 (M-46): `CoreTests c1600_4_ac16_examples_match_their_references` with `CALCU1600_REFERENCES=1`: each bundled example loaded through the library's loader and compared with its M-4 reference (display, BASIC and machine-code areas, paper); `Calculat`, `ce150_demo` (paper prefix) and biorhythmus' cursor row excepted, the examples that wait for LIB-7 listed | Longer | 81 s (28 compared, 4 waiting) |
| `CalcU1600Shots.<scenario>` | the nine screenshot scenarios of `docs/developer/screenshots/guide/` run as checks, into the build tree (M-46: `05-modules` and `08-presets` load presets that wait for LIB-7 and fail until M-47; the `method: system` shots need the Screen Recording and Automation grants); the check scenarios of `tests/gui/` (M-46: `lib6-ac54`; M-45): `lib8-ac36`, `lib8-ac38` (LIB-8 AC-36, AC-38), `lib10-ac41` (LIB-10 AC-41) | Longer | 39.6 s, the nine together (2.2–9.2 s each), all passed; the three of M-45 2.8, 3.6 and 1.7 s |
| `CalcU1600DapSmoke` | `tools/dap_smoke.py --app`: the DAP requests VS Code uses (M-45: the Qt `DapSession` on `app/dap/` and the library's run control, LIB-10 D16-12) | Longer | 26 s, passed twice in M-45; failed 3/3 earlier on 2026-11-14 and at the pin (no `stopped` event after the first `stepIn`), passed in the Part B run: possibly timing-dependent, open finding |
| `CalcU1600DebuggerSpeed` | `CoreTests lib10_speed_pc1600_full_speed_with_a_breakpoint` with `CALCU1600_SPEED=1`: a PC-1600 for 20 s of emulated time at full speed, with and without a breakpoint, printed (LIB-10 R-1; M-45) | Longer | 0.5 s (0.22 s and 0.20 s measured) |
| `CalcU1600PacingRealClock` | `lib17_ac21_pacing_longrun_real_clock_pc1500a`, `_pc1600` with `CALCU1600_PACING_LONGRUN=600`, on the library's app session (M-31) | Checkpoint | 20 min (by design) |
| `CalcU1600QtPacingProbe` | `CalcU1600Qt --pacing-probe 60` (non-release test hook): a PC-1600 keeps one minute in the Qt6 frontend (LIB-17 AC-22, P16-3; M-31). The Web half: `frontends/web/tests/session.spec.js` `lib17_ac22` with `CALCU1600_WEB_PACING=1` | Checkpoint | 1 min each; run once in M-31: Qt6 −8 ms, Web −2 ms over 60 s |
| `CalcU1600GuideScreenshots` | `tools/make_screenshots.sh`: the User Guide images into `docs/images/` | Checkpoint | not measured |

`CoreTests` by file (`products/calc-u-1600/tests/`, `app/tests/`; every
`TEST` is Frequent except where the table says otherwise; a `TEST` that
needs ROMs that are missing reports `SKIP`):

| File | `TEST`s | File | `TEST`s |
|---|---|---|---|
| `basic_fastloader_tests.cpp` | 12 (C1600-2 AC-4–6, EC-3, EC-5; M-44) | `pc1600_bank_tests.cpp` | 23 |
| `basic_program_source_tests.cpp` | 8 | `pc1600_basicloader_tests.cpp` | 15 (C1600-2 AC-4–6, EC-3; M-44) |
| `basictyper_tests.cpp` | 8 | `pc1600_basictyper_tests.cpp` | 12 |
| `basic_typing_tests.cpp` | 11 (C1600-2 AC-7 on the typing engine, against the old typer's memory; M-44) | `loader_memory_tests.cpp` | 7 (C1600-2 AC-14; M-44) |
| `program_files_tests.cpp` | 11 (C1600-2 AC-1–3, EC-1, EC-2, EC-4; M-44) | `app/tests/loader_part_tests.cpp` | 11 (C1600-2 AC-13, AC-22, AC-24, AC-25, EC-6; M-44) |
| `battery_card_instance_tests.cpp` | 19 | `pc1600_ce150_tests.cpp` | 5 |
| `ce150_tests.cpp` | 9 | `pc1600_host_drive_tests.cpp` | 7 |
| `ce151_tests.cpp` | 3 | `pc1600_keyboard_display_tests.cpp` | 38 |
| `ce155_tests.cpp` | 3 | `pc1600_machine_tests.cpp` | 33 |
| `ce158_tests.cpp` | 17 | `pc1600_phase54_tests.cpp` | 3 |
| `ce1600f_tests.cpp` | 24 | `pc1600_preset_tests.cpp` | 39 |
| `ce1600p_tests.cpp` | 5 | `pc1600_program_placement_tests.cpp` | 12 |
| `ce161_tests.cpp` | 4 | `pc1600_slot_module_tests.cpp` | 13 |
| `ce1638_tests.cpp` | 2 | `pc1600_slot_ram_tests.cpp` | 5 |
| `ce163f_tests.cpp` | 12 | `pc1600_tape_tests.cpp` | 3 |
| `ce502b_tests.cpp` | 2 | `peripherals_tests.cpp` | 8 (LIB-2 AC-28–32, AC-34; M-26) |
| `connector_tests.cpp` | 14 | `piezo_sampler_tests.cpp` | 12 |
| `debug_target_tests.cpp` | 31 (M-45: today's cases as `lib10_ac44_…` on `machine::Debugging`; `lib10_ac36_`, `ac37_`, `ac52_`; the expression cases moved to `dap_layer_tests.cpp`) | `preset_tests.cpp` | 44 |
| `descriptor_tests.cpp` | 8 (LIB-1 AC-34–41; M-26) | `presetloader_trace_tests.cpp` | 7 (the core's file mode for today's tools, `lib10_ac42_tools_…`; M-45) |
| `disasm_tests.cpp` | 15 | `program_file_tests.cpp` | 11 |
| `golden_runs_tests.cpp` | 1 (the golden replays through the machine interface; `CALCU1600_GOLDEN_RECORD=1` records new references only) | `run_control_tests.cpp` | 2 (`lib10_ac44_step_over_a_call`, `_data_breakpoint_and_pause` on the library's run control; M-45) |
| `harness_tests.cpp` | 13 (`harness_drives_a_sharp_machine`: the exit check of M-28, LIB-11 T14, D-1; LIB-18's model and variant; C1600-2 AC-21, the `pc1600` loader commands, M-44; M-45: `lib8_ac32_harness_…`, `lib10_ac40_harness_…`, `lib10_ac52_harness_peek_bank_and_vbank`; `SKIP` without the ROMs) | `debugger_tests.cpp` | 19 (LIB-10 1600: AC-38, AC-39, AC-40, AC-42, AC-43, AC-23 and R-1; M-45) |
| `host_directory_drive_tests.cpp` | 25 | `sc7852_tests.cpp` | 39 |
| `inspector_tests.cpp` | 13 | `sharp_machine_tests.cpp` | 28 (C1600-1 AC-1–21, EC-1–3; M-26) |
| `job_tests.cpp` | 2 | `tape_deck_tests.cpp` | 11 |
| `session_tests.cpp` | 4 (LIB-3 AC-35, AC-37, AC-38, ledger 1600-9 on the library's app session; M-31) | `dap_layer_tests.cpp` | 14 (C1600-3 AC-12, AC-14, AC-23: the DAP's layer in `app/dap/`; M-45) |
| `key_paste_tests.cpp` | 8 (the typing profile and engine; M-36) | `tc8576f_tests.cpp` | 20 |
| `lcd_screenshot_tests.cpp` | 5 | `app/tests/serial_links_tests.cpp` | 2 (LIB-2 AC-33; M-26) |
| `lcd_text_tests.cpp` | 21 | `app/tests/shared_app_session_tests.cpp` | 11 (SharpAppPart on the library's session, M-31; the loader cases moved to `loader_part_tests.cpp`, M-44) |
| `lh5801_tests.cpp` | 54 (the key queue's three went, M-36) | `app/tests/shared_bay_tests.cpp` | 4 |
| `lh5803_tests.cpp` | 8 | `app/tests/shared_geometry_tests.cpp` | 2 (the plotter paper's three moved to `plotter_paper_tests.cpp` as `lib8_ac37_…`, M-45) |
| `listing_tests.cpp` | 10 | `app/tests/shared_host_key_map_tests.cpp` | 3 (`SharpKeyboardMap`; M-36) |
| `machine_code_file_tests.cpp` | 39 (C1600-2 AC-8–12, AC-23; M-44) | `app/tests/shared_machine_session_tests.cpp` | 7 |
| `memory_card_tests.cpp` | 47 (the 8 YAML reader cases moved to the library in M-24) | `app/tests/shared_pacing_tests.cpp` | 2 (LIB-17 AC-20; the sink side of the sound; M-31) |
| `pacing_longrun_tests.cpp` | 4, the "after" test of LIB-17 AC-21 (M-31): `lib17_ac21_pacing_longrun_scripted_pc1500a`, `_pc1600` Frequent; `lib17_ac21_pacing_longrun_real_clock_pc1500a`, `_pc1600` Checkpoint (return at once without `CALCU1600_PACING_LONGRUN`) | `app/tests/shared_preset_catalog_tests.cpp` | 4 |
| `pc1500_tape_tests.cpp` | 2 | `app/tests/shared_settings_tests.cpp` | 8 (LIB-9 AC-26–32; M-30) |
| `rom_lookup_tests.cpp` | 9 (LIB-12 AC-11–14, EC-1; M-27) | `typing_tests.cpp` | 15 (LIB-5, M-36) |
| `paper_replies_tests.cpp` | 1 (`lib8_ac39_sharp_plots_turned_into_paper_replies`: the `.paper` files of M-8 turned into `paper marks` replies, compared with the committed `*.<plotter>.paper-marks`; `CALCU1600_PAPER_REPLIES=1` writes them; LIB-8 AC-39, M-39) | `plotter_paper_tests.cpp` | 9 (LIB-8 AC-32–34, AC-37; M-45) |
| `paper_session_tests.cpp` | 6 (LIB-8 AC-35, AC-36 in the app session; M-45) | | |

## Not in a group

Tools and one-time checks, not tests that run again (the inventories,
`docs/inventory/calc-u-59.md` and `calc-u-1600.md`, "Tests"):

- The harness `calcu` itself (`library/harness/cli/`; LIB-11): a tool, tested
  through `calcu_test` and the product entries above.
- Calc-U-59: the goldens generator `tools/goldens/` (regenerated when a
  driver changes), `tools/headless/`, `tools/agentic/` (harnesses),
  `tools/check_parity.sh` (a branch check), Qt `--grab` (by hand).
- Calc-U-1600: the CLIs `pc1500_cli`, `pc1600_cli` and the probes
  (harnesses and diagnostics), `dev/loader-matrix/`, `dev/tape-matrix/`
  (one-time verifications), `dev/hardware-checks/` (programs for real
  units).
