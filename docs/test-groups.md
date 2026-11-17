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
the CI jobs and `tools/run_tests.sh` need no filter. A product, and the
library, registers each entry with `calcu_add_test(<name> <group> COMMAND …
[WORKING_DIRECTORY <dir>])` (the root `CMakeLists.txt`): the group is its
label, a Longer or Checkpoint entry is registered only with its option,
and it runs in the product directory unless it names another (the
library's own entries run in their build folder);
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

## Local Linux CI

`ci/linux/run.sh` runs the CI's Linux jobs in Podman on Ubuntu 24.04 with
GCC 13, natively on arm64 (ADR 0039): before dispatching CI, and after a
change a GCC build may take differently from Apple clang. It is not a step's
check (a step is green by the build and the Frequent group).

| Job | CI job | |
|---|---|---|
| `library` | `library` (both workflows call `library.yml`) | default |
| `core-59`, `core-1600` | `core-tests`: build and the Frequent group | default |
| `qt-59`, `qt-1600` | `linux` (a matrix in both workflows) up to the build | default |
| `web-59`, `web-1600` | `web` up to the tests, each product's Emscripten | when named |

`ci/linux/run.sh core-1600` runs one job. The working tree is copied into a
volume without what git ignores (but the ROMs), so builds never write into
the checkout. The image follows `ci/linux/Containerfile`, which mirrors the
workflows' install steps and changes with them. Measured 2026-11-16 (M4 Pro,
machine `calcu` with 12 CPUs and 16 GB): 8.7 min cold with the image build,
1.6 min warm.

## Library

| CTest entry | What | Group | Measured (M4 Pro, 2026-11-14) |
|---|---|---|---|
| `calcu_library_tests` | 959 `TEST`s (M-58: `shell/tests/settings_dialog_tests.cpp` `also_shown_key_in_topic_and_set`, SettingsPart's `alsoShown` for C59-8 AC-7; M-57: LIB-14 §10.3, the shell for a window without a menu bar and the product data the page reads, `shell/tests/menu_model_tests.cpp` `more_items_are_the_items_without_a_control`, `more_cold_boot_asks_first`, `more_follows_availability`, `media_place_item_only_with_its_media_place`, `no_settings_item_without_settings`, `bar_model_tests.cpp` `bar_without_menu_bar_keeps_reset_preset_settings_and_more`, `settings_dialog_tests.cpp` `choice_row_with_labels_offers_only_those`, `choice_value_not_listed_shown_as_written`, `value_forms_tests.cpp` `shell_product_form_has_links_and_copy_screen_flag`; M-55: the library additions of LIB-7 §11, `ac17_kind_replace_question_words`, `ac16_kind_without_name_and_save_never_offers_it`, `ac12_reread_before_write_hands_current_file_as_source`, `ac1_shelf_and_writer_through_the_hosts_file_access`, `ac8_slot_without_media_listed_by_the_listing`; M-56: `debugger/tests/start_tests.cpp` `c59_4_ac17_fire_start_outside_an_advance`, `RunControl::fireStart`; Part D simplification S-5: `shell/tests/value_forms_tests.cpp`, `opening/tests/start_file_tests.cpp`; M-42, M-43, M-41, M-40, M-39, M-38, M-37, M-36; 21 of them harness attach, `library/harness/attach/tests/` (M-50: on `LocalStream`, the one-client byte listener the DAP's `serve --dap` shares) (M-49: `attach_registry_matches_serve`, `attach_bare_wait_ends_when_idle`), with `CALCU_ATTACH`, on by default in a POSIX tree with the tests; real sockets on 127.0.0.1 and a host loop on the real clock, about 3 s; M-33): the library's own tests with the machines built for the tests (`library/machine/tests/TestMachines.*`) and the product declared for the settings tests (`library/settings/tests/TestProduct.hpp`), no product code (LIB-1 AC-24; LIB-1 design §2a); also run alone by the CI job `library` of both product workflows. The pacing tests run on a scripted wall clock (`library/pacing/tests/TestSliceRunner.hpp`), never the host's; so do the session tests (`library/sessions/tests/ScriptedClock.hpp`), with the test frontend and the test product part of `library/sessions/tests/`. LIB-2's host links are tested here with an injected link factory (`peripherals/tests/host_links_tests.cpp`), on every platform; the ROM lookup's tests write their ROM files into a temporary folder (`library/roms/tests/TestRomFiles.hpp`); the harness tests run the product made for them (`library/harness/tests/TestProduct.*`: `test-seg` offering the machine state, `test-dot`) in virtual time, each run in a temporary run folder; LIB-18's tests run `library/startup/tests/TestProductMachines.hpp` (the test product, its settings and a session rig, paced and in virtual time) with the stand-in for LIB-6's loader (`TestPresetLoader.hpp`); LIB-5's typing tests run `library/typing/tests/TypingRig.hpp` (`test-type`, a test-seg with a scripted key readiness, a key log and typing support, `TypingTestMachine.*`; the test profile and map, `TestProfiles.hpp`), paced and in virtual time; LIB-6's preset tests run `library/presets/tests/PresetRig.hpp` (an app session with PeripheralsPart, PresetsPart, StartupPart and TypingPart; test-seg with the program of `TestSegProgram` and the memory `test.memory`) and the rules and keywords built for the tests (`TestPresetRules.hpp`), paced and in virtual time, with presets written into temporary folders and the fixtures of `library/presets/tests/fixtures/` (`bad/`, one file per class of error; `bundle/`; `good/`); LIB-8's paper tests run the harness's test product (which got the test printers TD-20 and TP-4 of the paper test catalogue, `paperTestCatalogue()`, and a PaperPart; M-39) and `library/paper/tests/PaperTestKit.hpp` (a printer driven call by call), with the descriptions of the products' printers and the fixtures of `library/paper/tests/fixtures/` (three `.marks` from the converters of AC-39 and five reference pictures compared byte for byte; `CALCU_PAPER_PICTURES=1` writes them anew); LIB-7's media tests run `library/media/tests/MediaFixture.hpp` (an app session with PeripheralsPart, PresetsPart, StartupPart, ControlPart and MediaPart; the media test catalogue `mediaTestCatalogue()`: the test machines with their media places on, `TestBuilder::media`, plus `test-memory-module` and `test-rom-module`; the kinds of `TestKinds.*`, in `calcu_test_machines`; a save folder and a bundled folder of each test's own, paced and in virtual time; M-41); LIB-13's opening tests run `library/opening/tests/TestOpeners.hpp` (stand-ins for LIB-6 and LIB-7 that record their calls, the file kinds `test.listing` and `test.blob`, on LIB-3's test session `Rig`) and, through the real parts, LIB-7's `MediaFixture.hpp` with an `OpenPart` (`FixtureOptions::moreParts`, a settings file of the test's own for EC-7), paced and in virtual time; upload folders are temporary (M-42) | Frequent | 0.64 s (M-28, 2026-11-15, RelWithDebInfo; most of it the five-hour scripted runs of LIB-17) |
| `calcu_library_pty_tests` | 4 `TEST`s of `library/peripherals/tests/pty_tests.cpp`: LIB-2's host links on real pseudo-terminals in a temporary folder for serial links (AC-18 to AC-20, EC-7); a program of its own (with the test machines), registered only where `CALCU_HAS_PTY` is set (macOS, Linux), so that it can move to Longer without a design change (LIB-2 design §8) | Frequent | 0.8 s (M-23, 2026-11-15) |
| `calcu_cli.<case>` | LIB-11 T13: `calcu_test` (the harness with the test product only) in a process of its own, run by `library/harness/tests/cli/cli_cases.cmake` with `cmake -P`: argv, standard input and output, exit status; eight entries, `ac1_product_by_name_or_short_form_any_case`, `ac2_one_shot_runs_in_order_and_exits_zero`, `ac2_first_refusal_exits_nonzero_with_reason`, `ac3_serve_one_reply_per_line_until_quit`, `ec3_serve_ends_at_end_of_input`, `ec4_unknown_product_lists_products` (M-28), `ac34_check_preset_exit_status` (LIB-6 AC-34, M-38), `ac50_refused_expect_ends_one_shot_nonzero` (LIB-10 AC-50, M-40) | Frequent | 0.1 s, the eight together |
| `calcu_preset_check.LibraryPresetFixtures` | LIB-6 L14 (design D-20): `calcu_add_preset_check()` (`library/presets/PresetChecks.cmake`, run by `RunPresetCheck.cmake` with `cmake -P`): `calcu_test check-preset` over `library/presets/tests/fixtures/good/*` and a template filled in by `fixtures/fill.cmake`; fails when a file fails. The products' checks of AC-43 and AC-51 use the same function from M-46 and M-54 (M-38) | Frequent | 0.05 s |
| `calcu_serve_python` | LIB-11 T15: `library/harness/python/tests/test_calcu_serve.py`, the Python helper for `calcu serve` against `calcu_test` (with the reply timeout, 2026-11-17) (with `uv run --no-project python -I`, else Python 3; M-28) | Frequent | 0.05 s |
| `LibraryPlatformStoreCheck` | `library/settings/tests/check_platform_stores_test.sh`: the self-test of `library/settings/check_platform_stores.sh`, the source check the products run over their frontends (LIB-9 design T-14); not registered on Windows | Frequent | 0.03 s |
| `calcu_qt6_tests` | 64 `TEST`s (Part E simplification E-4: 7 of `library/qt6/tests/file_opening_tests.cpp`, LIB-13 AC-6, AC-20, AC-21, EC-5 and LIB-7 D-6 on `FileOpening` and `AskedSend`; M-57, LIB-14 L14: `faceplate_origin_moves_the_hit_map`, `faceplate_press_the_product_takes_passes_through`, `about_shows_the_window_icon` in `shell_widgets_tests.cpp`; S-5: 4 of `library/qt6/tests/host_keys_tests.cpp`, LIB-5 AC-7, AC-10, EC-1): 10 of `library/qt6/tests/paper_panel_tests.cpp` (LIB-8 T13, T14: AC-12 to AC-16, AC-18 to AC-20 on the Qt6 paper panel and `ClipboardService`, an app session with PaperPart and the test printers; M-39), 3 of `media_components_tests.cpp` (LIB-7 L11: AC-8, AC-9, AC-16 on `MediaPicker` and `NameAndSaveDialog`, without a window; M-41), 1 of `run_control_buttons_tests.cpp` (LIB-10 L16: AC-18 on `RunControlButtons`, the rule's table and an app session with the DebuggerPart, a held step's real 250 ms repeats; M-40); 28 of `shell_widgets_tests.cpp` (LIB-14 L7: AC-1 to AC-21, AC-23, AC-24, AC-43, EC-2, EC-4, EC-6 on the shell's widgets, the test window `tests/TestWindow.*` over `TestShellPart` and SettingsPart on a scripted clock; the shortcut and touch cases with Qt Test, skipped without it; M-43); 1 of `shell_widgets_tests.cpp` for LIB-7 AC-29 (`lib7_ac29_qt6_file_menu_without_media_items`: no media items in the Qt6 File menu, LIB-7 OQ-4; M-42); 5 of `attach_host_tests.cpp` (LIB-14 L8, AC-44 support; M-49: `qt_attach_host_preset_key_type`: harness attach in `SessionHost` with LIB-11's test product, real sockets on 127.0.0.1 and the 60 Hz timer, about 1 s; only with `CALCU_ATTACH`; M-43): the Qt6 component set's one test program (`calcu_qt6`, `QtTestMain.cpp`; LIB-10, LIB-7 and LIB-14 add files to it), `QT_QPA_PLATFORM=offscreen`; registered only in a tree with `CALCU_FRONTEND_QT` where Qt6 Widgets is found (not in `build/tests`, which is configured with `-DCALCU_FRONTEND_QT=OFF`) | Frequent | 2.7 s (M-43; 1.8 s of it the paper toast's real wait, about 0.6 s the attach host on the real clock) |
| `calcu_qt6_scenarios` | 9 `TEST`s of `library/qt6/tests/shell_scenarios.cpp` (LIB-14 L10: the scenarios of AC-25 under their IDs, `ac1_the_menus`, `ac4_a_shortcut`, `ac6_the_platforms_places`, `ac8_the_bar`, `ac11_the_settings_dialog`, `ac15_loading`, `ac17_a_notice`, `ac20_help_opens`, `ac24_a_key_under_the_pointer`, on the test window, widgets found by the object names of LIB-14 §3.10; M-43); offscreen; registered with `calcu_qt6_tests` | Frequent | 0.2 s (M-43) |
| `LibraryShellNoProduct` | `library/qt6/tests/check_no_product_code.sh`: no include from `products/` and no model, peripheral or product name in the code (comments aside) of `library/shell`, `library/qt6` and `library/web/components` (LIB-14 L11, AC-22; M-43); not registered on Windows | Frequent | 0.2 s (M-43) |
| `LibraryHelpTool` | `python3 -m unittest discover -s helptool/tests -t .` in `tools/`: the help tool's cases (LIB-16 L1 to L8, `tools/helptool/tests/test_*.py`: manifests and the AC-1 contract, the asset and `toc.json`, anchors against `library/help/anchor_cases.txt`, links, `chapter:` links, the checks of AC-2 to AC-5, AC-7, AC-14, EC-1 to EC-3), each on a copy of the fixture help tree `tools/helptool/tests/fixtures/good/`; registered when Python 3 is found (M-52) | Frequent | 0.5 s (M-52) |
| `LibraryWebNodeTests` | `node --test library/web/tests/build.test.js library/web/tests/shell_model.test.js library/web/tests/host_keys.test.js`: 5 tests of the Web frontend shell's build functions (`library/web/build/dist.js`: MANIFEST, `files.json`, the service worker stamp, one global in `calc.js`, the CSS scope check, the version from `release.yaml` (Part E, E-6); LIB-15 AC-14, AC-15, AC-17; M-29) and 40 of the React set's shell models (`library/web/components/shellModel.js`, the mirror of the C++ model cases under the same names, on `library/shell/tests/inputs.txt` and `hit_cases.txt`; LIB-14 L1 to L6; M-43) and 4 of the host keys (`library/web/components/hostKeys.js`, LIB-5 AC-7, AC-10, EC-1; S-5); registered when `node` is found; also run by the CI job `library` | Frequent | 0.3 s (M-43, 2026-11-15) |
| `LibraryWebPlaywright` | `library/web/tests/run-tests.sh`: builds the product made for the tests (`library/web/tests/product/build.sh`: `calcu_web_test_product` in `build/web`, then `Test-A`, `Test-B`, a second release of `Test-A` and `Test-M` (with LIB-7's MediaPart, M-42) in `build/web/library/web/test-dist/`), then Playwright: `hosting`, `storage`, `embed` in Chromium (root, sub-path) and WebKit (root), `offline` in Chromium and WebKit, `prerender` in Node (LIB-15 AC-1 to AC-18, EC-1 to EC-5; M-29); `paper` (`tests/paper.spec.js`, LIB-8's React paper panel on the test product with TD-20 and TP-4: AC-12 to AC-15, AC-19 to AC-21; 9 cases) in Chromium and WebKit (M-39); `run-control` (`tests/run-control.spec.js`, LIB-10 L12, L16: the React run-control buttons on the rule's table and the test product's DebuggerPart, AC-18; an ended trace as a download, AC-26; 2 cases) in Chromium and WebKit (M-40); `shell` (`tests/shell.spec.js`, LIB-14 L9: the React set's shell on the test product with `shell=1`, TestShellPart and SettingsPart; the AC-25 scenarios `ac1`, `ac4`, `ac6`, `ac8`, `ac11`, `ac15`, `ac16`, `ac17`, `ac19`, `ac20`, `ac22`, `ac24`, `ac43` and LIB-7's `ac29`; 14 cases) in Chromium and WebKit (M-43); `media` (`tests/media.spec.js`, LIB-7 L11: the React media components on `Test-M` with `shell=1`: `ac29` File menu and the persistent file system, `ac8`, `ac9`, `ac16`, `ac31`; 6 cases) in Chromium and WebKit (M-42); 138 cases, 2 skipped; M-57: `faceplate origin and a press the product takes`, `storage banner shows the library's sentence while not kept`, 140 passed, 2 skipped (`ec1` runs in the Chromium root project only); not registered on Windows; in CI in the `web` job of both product workflows, with each one's Emscripten (LIB-15 D-8) | Longer | 35 s, plus 10 s for the build with `build/web` built (M-29, Homebrew Emscripten 7.0) |

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
| `harness/tests/run_tests.cpp` | 20 (LIB-11 AC-2, AC-6, AC-8, AC-9, AC-21, AC-24, EC-1, EC-3, EC-6; M-28, M-35; M-51: `prepare_runs_before_session_parts`, D-11) |
| `harness/tests/serve_addon_tests.cpp` | 2 (C1600-3 T13, AC-22: a product's option of `serve`: its tasks in order with the lines, emulated time only while it wants it, a start that is refused; M-50) |
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
| `paper/tests/picture_tests.cpp` | 4 (LIB-8 AC-16, AC-17, the reference pictures of the test printers; M-39; the PC-100C's went to Calc-U-59 in M-53) |
| `paper/tests/paper_part_tests.cpp` | 16 (LIB-8 AC-6 to AC-9, EC-2 to EC-4, EC-7; M-39) |
| `paper/tests/copy_cut_tests.cpp` | 5 (LIB-8 AC-16, AC-18 to AC-20, EC-5; M-39) |
| `paper/tests/paper_step_tests.cpp` | 4 (LIB-8 AC-22; M-39) |
| `media/tests/names_tests.cpp` | 8 (LIB-7 AC-2 to AC-4; M-41) |
| `media/tests/shelf_tests.cpp` | 11 (LIB-7 AC-1, AC-5, AC-46, EC-3; M-41; M-47: a file that is no medium of its kind) |
| `media/tests/place_tests.cpp` | 5 (LIB-7 AC-6; LIB-1 AC-40 with D-1, `checkMediaPlaces`; M-41) |
| `media/tests/topic_tests.cpp` | 4 (LIB-7 AC-7; M-41) |
| `media/tests/picker_tests.cpp` | 9 (LIB-7 AC-8 to AC-10; M-41) |
| `media/tests/autosave_tests.cpp` | 18 (LIB-7 AC-11 to AC-15, EC-1, EC-2, EC-8; M-41; M-47: a place that writes when asked) |
| `media/tests/save_tests.cpp` | 13 (LIB-7 AC-16 to AC-20, EC-7, EC-9; M-41) |
| `media/tests/preset_media_tests.cpp` | 20 (LIB-7 AC-21 to AC-27, EC-5, EC-6; LIB-6 AC-3 by `file:` and the bundle check of bundled media; LIB-18 AC-2, AC-7 media clauses; M-41) |
| `media/tests/saved_media_tests.cpp` | 9 (LIB-7 AC-1, AC-30, AC-31, EC-4; M-41) |
| `opening/tests/classify_tests.cpp` | 15 (LIB-13 AC-1, AC-2, AC-10, EC-1, EC-5; M-42) |
| `opening/tests/open_tests.cpp` | 25 (LIB-13 AC-3, AC-4, AC-7, AC-8, EC-2, EC-3, EC-4, EC-6; the request's Value form; M-42) |
| `opening/tests/ways_tests.cpp` | 4 (LIB-13 AC-5, AC-6; M-42) |
| `opening/tests/real_parts_tests.cpp` | 3 (LIB-13 AC-1, AC-4 through LIB-6's and LIB-7's parts, EC-7; M-42) |
| `opening/tests/start_file_tests.cpp` | 1 (LIB-13 AC-21, `startFile`; from Calc-U-1600's `opening_tests.cpp`, S-5) |
| `shell/tests/menu_model_tests.cpp` | 13 (LIB-14 AC-1 to AC-7, AC-19, EC-4, LIB-7 AC-29's menu items; M-43) |
| `shell/tests/bar_model_tests.cpp` | 10 (LIB-14 AC-8 to AC-10, AC-23, AC-42, EC-5; M-43) |
| `shell/tests/settings_dialog_tests.cpp` | 9 (LIB-14 AC-11 to AC-14, EC-3, D-19; M-43; `also_shown_key_in_topic_and_set`, M-58) |
| `shell/tests/loading_notice_tests.cpp` | 7 (LIB-14 AC-15, AC-17, EC-1, EC-2, EC-7, D-7, N4; M-43) |
| `shell/tests/hit_map_tests.cpp` | 4 (LIB-14 AC-24, AC-43, EC-8; M-43) |
| `shell/tests/help_about_tests.cpp` | 2 (LIB-14 AC-20, AC-21; M-43) |
| `shell/tests/inputs_fixture_tests.cpp` | 1 (the JS mirror's inputs from the payloads; M-43) |
| `shell/tests/value_forms_tests.cpp` | 4 (LIB-14 D-31, D-32: the page's Value forms of `ShellProduct`, `DialogSpec`, `HitMap`; S-5) |
| `presets/tests/list_tests.cpp` | 6 (LIB-6 AC-31 to AC-33, EC-9; M-38) |
| `presets/tests/startup_tests.cpp` | 3 (LIB-18 AC-4, EC-1, EC-6 with the real loader; M-38) |
| `presets/tests/typing_tests.cpp` | 3 (LIB-5 AC-14, AC-15, AC-17 in a preset; M-38) |
| `startup/tests/build_tests.cpp` | 11 (LIB-18 AC-1, AC-2, EC-4; M-35; M-51: the hook `ProductMachines::startUp` of the adapter SharpBuildColdBoot gone, `ac1_one_build_…` no longer counts it) |
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
| `CalcU59Tests` | 597 `TEST`s (M-60: LIB-16 59, `help_tests.cpp` (`lib16_ac14_settings_keys_file_matches_declaration`, `lib16_ac14_every_declared_key_is_in_the_keys_file`, `lib16_ac2_help_names_cover_the_catalogue`, `lib16_ac9_tutorial_presets_load`: `help/settings-keys.txt`, `help.yaml`'s `names:`, the six tutorial presets; about 1 s); M-59: C59-9 and LIB-11 59, `harness_product_commands_tests.cpp` (`c59_9_ac1_…` to `c59_9_ec6_…`, the `c59` commands of C59-9), `machine_state_tests.cpp` (`c59_9_ac15_…`), the `lib11_ac14_…` to `lib11_ac17_…` cases of `harness_tests.cpp`; M-58: C59-8, `swift_shell_tests.cpp` (8, `c59_8_ac18_…`, `c59_8_ac1_…`, `c59_8_ac3_…`, `c59_8_ac5_…` (2), `c59_8_ec1_…`, `c59_8_ac7_…`, `c59_8_ac19_…`: the shell's models with the Swift frontend's traits on the session); `pointer_tracker` went with `cu59::PointerTracker`; M-57: LIB-4 59 and LIB-14 59, `control_state_tests.cpp` (`lib4_ac10_…` to `lib4_ac14_…`, `lib4_harness_state_control_state_has_paper_panel_and_card_reader`), `shell_product_tests.cpp` (`lib14_ac26_shell_product_complete`, `lib14_ac32_…`, `lib14_ac30_…`, `lib14_ac24_faceplate_hit_map`, `lib14_ac19_roms_missing_topic_without_a_machine`), `web_part_tests.cpp` `lib14_web_shell_product_per_layout`, `lib14_web_faceplate_map_and_heatmap_grid`; M-55: C59-7 and LIB-7 59, `magnetic_card_tests.cpp`, `card_flows_tests.cpp`, `media_tests.cpp`, the rig `tests/CardRig.hpp`, about 17 s more of ROM-driven card cases; M-56: C59-4 and LIB-13 59, `debug_panel_tests.cpp`, `asm_overlay_tests.cpp`, `opening_tests.cpp`, the `c59 asm|dump|log-level|heatmap` and `open` cases of `harness_tests.cpp`; M-54: LIB-6 59, LIB-18 59, C59-2, C59-3, LIB-5 59's preset clauses: `preset_content_tests.cpp`, `ti_presets_tests.cpp`, `machine_control_tests.cpp`, `constant_memory_tests.cpp`, the rigs `tests/AppRig.hpp` and `tests/TIPresetRig.hpp`; M-53: LIB-8 59 and LIB-10 59, `pc100c_paper_tests.cpp`, `debug_target_tests.cpp`, `debugger_tests.cpp`, `trace_tests.cpp`; M-39: `tests/paper_replies_tests.cpp`, LIB-8 AC-39; M-36; `tests/typing_tests.cpp`: LIB-5 AC-20–27, T59-0): unit and ROM-driven integration tests of the core and the app layer, the machine interface (C59-1, LIB-1 59), the ROM lookup (LIB-12 59), the PC-100C and the module slot (C59-6, LIB-2 59), the settings file (LIB-9 59), the golden replays through it (table below), the harness product entry (LIB-11, M-28; its model switch through LIB-18, `harness_switches_to_a_ti58`, M-35), Calc-U-59 on the library's app session and the session thread (LIB-3 59, LIB-17 59, C59-5; M-33), the product's flows on it and the Web part (M-34) | Frequent | 13 s (M-33); 7 s (M-34, RelWithDebInfo); 8.3 s (M-53); 10 s (M-54); 21 s (M-56, in a parallel `ctest -j8`; the debug panel's and opening's session cases); 32 s alone, 41 s in `ctest -j8` (M-59, RelWithDebInfo) |
| `CalcU59TraceReader` | `tools/read_trace.py --json` reads the trace `CalcU59Tests` wrote (since M-53 the trace recorder's, `lib10_ac33_header_ti59_version_2_and_model`); after `CalcU59Tests` (`DEPENDS`); its 4 MB of JSON go to `/dev/null` (`sh -c`), a failure shows on standard error and in the exit status (Part E simplification E-11: CTest took 15 s to take the output in) | Frequent | 0.1 s (E-11; 16.4 s before) |
| `CalcU59QtTests` | 27 `TEST`s (M-58: `c59_7_ac15_qt6_fresh_card_written_and_listed`, `c59_7_ac12_qt6_card_read_by_its_corner`, C59-7 AC-28) of `products/calc-u-59/frontends/qt6/tests/` on the real window of Calc-U-59 put together from LIB-14's shell (`tests/QtFixture.*`: the window over the session on the ROMs compiled in as the app has them, the product's `examples/` as the bundled examples, the file dialogs and the browser replaced by recorders; on the real clock): since M-57 `window_tests.cpp`, the scenarios of LIB-14 AC-25 for this product under their IDs (`lib14_ac26_qt_menus_in_family_order`, `…_shortcuts_ctrl_o_r_shift_r`, `…_roles_settings_about_quit`, `lib14_ac27_bar_per_model`, `lib14_ac28_…` (2), `lib14_ac30_settings_sections_and_rows`, `lib14_ac31_…` (2), `lib14_ac32_…` (2), `lib14_ac33_three_columns_with_dividers` (the window's picture into `test-pictures/` of the build tree), `lib14_ac24_key_under_pointer_ti59_ti58`, `lib14_ac24_press_on_the_fitted_card_feeds_it`, `lib14_ac43_display_hold_full_speed`, `lib14_ac19_roms_missing_in_window` (`QtResourceBundle::hideForTests`)), LIB-6 AC-42 (`lib6_ac42_qt6_bundled_example_sum_shows_its_cue_card`), C59-4 AC-24 (`c59_4_ac5_qt6_…`, `c59_4_ac16_qt6_…`), `qt_attach_drives_calc_u_59` (harness attach on port 0, with `CALCU_ATTACH`); `qt_frontend_tests.cpp` (M-53, M-55, on the window since M-57): `lib8_ac28_qt6_print_adv_held_trace_switch_view_control`, `lib8_ac31_qt6_print_text_cut`, `lib10_ac34_qt6_calculator_and_cpu_buttons_follow_run_state`, `c59_7_ac13_buttons_and_eject_by_card_state`, `c59_7_ac14_dialog_lists_saved_cards_outdated_disabled_none_found`; `QtTestMain.cpp`, `QT_QPA_PLATFORM=offscreen` and a `HOME` of the build tree's (no settings of the user's); registered only in a tree with `CALCU_FRONTEND_QT` where Qt6 Widgets is found | Frequent | 4.2 s (M-53; about 1 s of ROM start-up per case); 22 s (M-57, 25 cases; about 1 s of ROM start-up per window) |
| `CalcU59NoOwnSession` | `tests/check_no_own_session.sh`: the Qt6, Swift and (M-34) Web frontends use no `EmulatorSession`, `EmulationRunner`, `TISessionController` or own `SessionController`; no `std::thread` in `app/` but `SessionThread`; the Swift bridge includes no `SessionController.hpp` and calls no tick; Qt6 sends no suspend or continue (LIB-3 AC-28, AC-31; C59-5 B2; M-33) | Frequent | 0.03 s |
| `CalcU59FrontendsDrawControlState` | `tests/check_frontends_draw_control_state.sh`: no file of the Qt6, Web and Swift frontends reads `hasCardReader`, `cu59::info(`, `c59.model`, `c59.web.model`, `allModules` or `MachineModel`, and `App/MachineModel.swift` is gone: they draw the model, the module, the printer and the card controls from the control state (LIB-4 AC-15, design §10.6 T59-6, D-19; M-57; the Swift frontend since M-58, C59-8) | Frequent | 0.02 s |
| `CalcU59HelpCheck` | `tools/help check calc-u-59 --quiet-warnings` (LIB-16 AC-1 to AC-5, AC-7, AC-9, AC-10, AC-14, AC-15, EC-1 to EC-3: the checks C1 to C9 over `library/help/` and `help/`); registered when Python 3 is found (M-60), with `CalcU59HelpBuild` by the root's `calcu_add_help_tests` (Part E, E-6) | Frequent | 0.3 s (M-60) |
| `CalcU59HelpBuild` | `tools/help build calc-u-59 --quiet-warnings --out <tree>/products/calc-u-59/help`: the help asset `Calc-U-59-help.zip` (LIB-16 AC-6, AC-8: the test suite builds both products' assets) (M-60) | Frequent | 0.4 s (M-60) |
| `CalcU59CoreSeam` | `tests/check_core_seam.sh`: no core-internal header outside `core/` and `tests/`, no machine call in the adapters (C59-1 AC-17; M-19) | Frequent | 0.02 s |
| `CalcU59OpenWithTypes` | `tools/check_open_with_types.sh`: the Qt6 app's Open With offers today's three preset extensions on macOS (`Info.plist.in`), Windows (`calc-u-59.iss`) and Linux (`calc-u-59.desktop`) (LIB-13 AC-14, design §10.6 E13-5; M-56) | Frequent | 0.03 s |
| `CalcU59NoOwnRomLookup` | `library/roms/tests/check_no_own_rom_lookup.sh`: no product source outside `app/ProductRoms.*`, the catalogue and the Qt6 Bundle looks for a ROM file (LIB-12 AC-2, design T7; M-24) | Frequent | 1.2 s |
| `CalcU59NoPlatformSettings` | `library/settings/check_platform_stores.sh` over `frontends/` with `tools/platform_stores_allowed.txt`: no setting in UserDefaults, `@AppStorage`, QSettings or `localStorage`; allowed: the trace folder's bookmark (AC-34) and the Playwright check that 2.3.0's entries are not read (LIB-9 AC-21, design T-24; M-32; the page's own entries went in M-34); not registered on Windows | Frequent | 0.02 s |
| `CalcU59HarnessRomMissing` | `tests/harness_rom_tests.py`: `calcu --product c59` on a ROM folder without one file (`CALCU59_ROMS_DIR`) ends with exit status 2 naming it, on the full set it starts (LIB-12 AC-7, AC-10; was `CalcU59HeadlessRomMissing` on `tools/headless/ti59.py` until M-59; with Python 3) | Frequent | 0.3 s |
| `CalcU59NoOldTools` | `cmake -P tools/check_no_old_tools.cmake`: `tools/headless/`, `tools/agentic/`, `core/TIAdapters.*` and `tests/headless_rom_tests.py` are gone; no source names `ti59headless`, `TI59HEADLESS_LIB`, `TIMemoryAdapter`, `memoryOf(` or includes `TIAdapters.hpp` (LIB-11 §11.6 H4; M-59) | Frequent | 0.05 s |
| `CalcU59CalcuRuns` | `cmake -DCALCU=… -P tests/calcu_runs.cmake`: `calcu --product c59 wait 1s then display-text` as a process, exit status 0 and the display (LIB-11 AC-15, §11.6 H5; also a step of every desktop job of `calc-u-59.yml`; M-59) | Frequent | 0.1 s |
| `CalcU59XcodeProjectCheck` | `frontends/swift/check_xcode_project.sh`: Xcode compiles only Swift and the bridge and links `CalcU59.xcframework` (C59-5, ADR 0013; M-15); the app bundles the 24 ROM files of the full set (LIB-12; M-24); only Debug takes incoming connections, with `Calc-U-59-Debug.entitlements` (C59-5 AC-10, AC-13; M-33); the XCFramework script phase declares its output (M-33) | Frequent | 0.02 s |
| `CalcU59NoAttachSources` | `frontends/swift/check_no_attach.sh --sources`: attach only for the Debug `macos` and `ios-simulator` slices in `build_xcframework.sh`, the bridge's and the Swift call sites under the Debug and platform guard (C59-5 AC-13, B1; M-33). The `--binary` half runs as a build phase of every Release and device build of the app | Frequent | 0.02 s |
| `CalcU59NoTestAids` | `frontends/swift/check_no_test_aids.sh`: the Swift app's sources hold no `--UITesting`, `isUITesting`, `btn-key-rs`, `keystroke-playback-status`, `--setting`, `--module`, `isKeystrokesPlaying` or `MachineModel` (C59-8 AC-21, LIB-4 AC-15; design T11, T12; M-58) | Frequent | 0.02 s |
| | **Frequent, Calc-U-59** | | **25.9 s** (M-32); 31.2 s wall alone with `-j8`, `CalcU59Tests` 30.8 s the longest (E-11, 2026-11-17, M4 Pro under other load) |
| `CalcU59WebPlaywright` | `frontends/web/run-tests.sh`: Playwright, Chromium (root, sub-path) and WebKit (offline); the servers are `library/web/tools/serve.py`, the offline server and the prerender sandbox `library/web/tests/dist-helpers.js`, `gotoReady` in `tests/helpers.js` (Part E, E-6); `rom-missing.spec.js` (LIB-12 AC-10, EC-3; M-24); since M-34 on the library's shell, with `session.spec.js` (LIB-15 AC-19, AC-22, EC-4, LIB-3 AC-31) and `storage.spec.js` (LIB-15 AC-20, AC-21, LIB-9 AC-23); `play-calculator.spec.js` host keys by the carried letter, two keys, a short click held 100 ms (LIB-5 AC-21–24; M-36); since M-56 `LIB-13 AC-12`, `AC-15`, `AC-16` (files through `open`) and `C59-4 AC-5`, `AC-16`, `AC-21` (the debug panel on its product topics); since M-57 the page on the React set's shell, `shell.spec.js` (the scenarios of LIB-14 AC-25 for this product under their IDs, `lib14_ac24` to `lib14_ac33`, the More button of the portrait layout, the screenshots of the three layouts in the test output; `lib7_ac36 file menu has import and saved media`), `rom-missing.spec.js` `lib14_ac19 roms missing in the window, help and about work` | Longer | 78 s (M-24); 78 s, 85 passed, 5 skipped (M-34); 91 passed, 5 skipped (M-36); 103 passed, 5 skipped (M-56); 119 passed, 5 skipped (M-55: the card scenarios on the media topic, `C59-7 AC-12`, `C59-7 AC-15`, `c59_7_ac13`/`ac14`/`ac26`, `lib7_ac36`, `lib7_ac33`, LIB-15 `ac20 cards survive a page load`); 149 passed, 5 skipped (M-57) |
| `CalcU59ReferenceRuns` | `CalcU59Tests c59_2_ac11` with `CALCU59_REFERENCE_COMPARE=1` (C59-2 AC-11, design §6 T10; M-54): each upgraded example through the harness entry in virtual time (`preset <file>`, then the bare `wait`), compared with the reference run of its 2.3.0 file of M-9 (`tests/reference/v2.3.0-examples/`) in the display, the partition, the raw RAM (120 registers) and the paper (`paper text`, `paper marks`); ticks, `Elapsed:`, the log and the `playback:` line are not compared; `historic/life.ti59` is only loaded; the four card examples also in the banks of their in-file and written cards (M-55); a difference is a finding, named with its reason in `tests/reference_runs_tests.cpp` (`kFindings`), never a new reference (user, 2026-11-15); `CALCU59_REFERENCE_EXAMPLES=<folder>` compares a draft folder | Longer | 7 s |
| `CalcU59RomMissingBuild` | `tests/rom_missing_build.sh`: roms/ without one file (default `TMC0583.txt`); the harness `calcu` on it (`CALCU59_ROMS_DIR`, since M-59; was the headless library) and a Web `dist/` built from it start without a calculator and name the file (LIB-12, Technical Requirements; M-24); not registered on Windows | Longer | 3 s (M-24; the Web build tree already built) |
| `CalcU59Scripts` | `tests/check_scripts.py` (C59-9 AC-13, T11; M-59): `tools/coverage.py build boot digit5 add` and `show` give 2.3.0's words of the three scenarios (`tests/reference/scripts/coverage-base.json`) without the key-detect range; `tools/fastmode_matrix.py label` gives 2.3.0's verdicts of the `label` cases (`fastmode-label-base.txt`) (with Python 3) | Longer | 6 s |
| `CalcU59UIRegression` | XCUITest plan `UIRegressionTests` on the `iphone` simulator; since M-33 with `AttachTests` (harness attach on port 59160: C59-5 S3, S4 Home and back, S5 full speed by a press on the display); `PacingTests` and `ScreenshotTests` skipped; M-58: `FamilyStructureTests` (C59-8 AC-16's scenarios, EC-2–EC-4, AC-5, AC-21's settings file read by path), every launch with `--settings-file` and attach on port 59162, no `--UITesting`; M-56: `DebugPanelTests` (C59-4 AC-5, AC-21; `ScreenshotTests.testC59_4_AC16AsmOverlay` in the Screenshots plan) | Longer | 447 s, 39 tests (iPhone 18 Pro Max, iOS 27.2; M-58; 178 s, 18 tests in M-33) |
| `CalcU59SwiftUnitTests` | `xcodebuild test -testPlan UnitTests` on the `iphone` simulator: `Calc-U-59Tests`, hosted by the app with `--settings-file unit-tests.ini`, a second session through the real bridge (C59-5 S1 `MessageOrderTests`, S2 `HostServicesTests`, `SuspendTests` AC-8, `FrameGapTests` AC-5/D-9; M-33; M-56: `OpeningTests` LIB-13 AC-15, AC-6; M-58: `ShellBridgeTests` C59-8 AC-6, AC-7, AC-9, AC-12, AC-13, AC-18, D-8) | Longer | 14 s of tests, about 40 s with the build (M-33) |
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
| `core_tests.cpp` | 17 (C59-1 AC-5; C59-6 AC-4, AC-5, M-25; LIB-8 AC-30 `lib8_ac30_print_gives_a_line`, `lib8_ac30_interrupt_rows_against_the_scan`, M-53) |
| `debug_target_tests.cpp` | 6 (LIB-10 AC-28: the TMC0501 target; M-53) |
| `debug_panel_tests.cpp` | 49 (C59-4 AC-1–13, AC-16–19, AC-21, AC-23, AC-25, AC-26, EC-1–5, named `c59_4_acN_…`: the product topics and entries of `TIDebugPart`, the CALCULATOR and CPU tabs, the heatmap, the LOG tab and the dumps, the ASM overlay's kind, box, Run and life, its file kind; the cases of `debug_model_tests.cpp:184-287` and `part_flows_tests.cpp`'s `part_asm_overlay` moved here; M-56) |
| `asm_overlay_tests.cpp` | 16 (C59-4 AC-14, AC-15, AC-20: the ASM overlay format 2.0 and the five shipped overlays against the instructions 2.3.0's reader gave them, `tests/fixtures/asm/*.words`; M-56) |
| `opening_tests.cpp` | 18 (LIB-13 AC-11, AC-13–16, AC-23, EC-7 on Calc-U-59, named `lib13_acN_…`: the file kinds of `app/TIFileKinds`, the keystroke program, the refused kinds, the start file's rule, the ways in; M-56) |
| `debugger_tests.cpp` | 21 (LIB-10 AC-29–32, AC-35 on the run control: levels, pause-on-start, breakpoints, the debugger steps of a preset, the freeze tests of 2.3.0 ported; M-53, M-54) |
| `descriptor_tests.cpp` | 9 (LIB-1 AC-26–33; M-19, M-25) |
| `harness_product_commands_tests.cpp` | 26 (M-59: C59-9 AC-1 to AC-11, AC-14, EC-2 to EC-6, the `c59` commands, each on a `calcu` run with its reply written out; `SKIP` without the ROMs) |
| `machine_state_tests.cpp` | 5 (M-59: C59-9 AC-15, the machine state of the TI-59, TI-58, TI-58C, with a PC-100C, a module and a card passing the reader; refused for other peripherals) |
| `harness_tests.cpp` | 25 (M-59: LIB-11 AC-14 to AC-17, `lib11_ac14_entry_runs_ti_app_part`, `lib11_ac14_every_function_of_the_table` (the exit check of M-59), `lib11_ac16_…`, `lib11_ac17_…`; M-56: C59-4 AC-22 `c59 asm`, `dump`, `log-level`, `heatmap reset`, no preset steps for them; LIB-13 AC-9 `open`; M-54: `harness_starts_with_ml_and_no_printer`, LIB-18 AC-18; C59-2 AC-9 `c59 partition|program|registers`; LIB-6 AC-34 `preset`, `wait`, `expect`, `screenshot`, `printer-trace`; `harness_drives_a_ti59`: the exit check of M-28, LIB-11 T14; the model switch, LIB-18; `key`, LIB-5; M-53: LIB-8 AC-23 `paper` and `cut-paper`, LIB-10 AC-31 `break`; `SKIP` without the ROMs) |
| `golden_runs_tests.cpp` | 2 (M-53: `lib10_ac33_bytes_equal_the_recorded_reference`, the trace recorder's file of `ti59-calculation.run` against the length and MD5 of the trace recorded on 2.3.0's path; the 18 golden replays through the machine interface and, since M-25, the places, C59-1 AC-4, C59-6 AC-3, AC-7, AC-9, AC-10; `CALCU59_GOLDEN_RECORD=1` records a missing reference only) |
| `machine_tests.cpp` | 26 (C59-1 AC-1–3, 6–16, 18, 19, EC-1–4; M-19) |
| `peripherals_tests.cpp` | 20 (C59-6 AC-1, 2, 4–6, 8–11, EC-1–3; LIB-2 AC-24–27; M-25) |
| `pacing_longrun_after_tests.cpp` | 2, "after" (LIB-17 AC-18; M-33): `lib17_ac18_pacing_longrun_scripted_ti` Frequent; `lib17_ac18_pacing_longrun_real_clock_ti` Checkpoint |
| `pacing_tests.cpp` | 7 (LIB-17 AC-15–17; M-33) |
| `session_tests.cpp` | 10 (LIB-3 AC-29, AC-30, AC-32, AC-33; C59-5 AC-8; M-33) |
| `session_thread_tests.cpp` | 10 (C59-5 AC-4–6, AC-8, H1–H3; real clock, about 4 s; M-33) |
| `paper_replies_tests.cpp` | 1 (`lib8_ac39_ti_printouts_turned_into_paper_replies`: the printouts of the M-9 references turned into `paper text` and `paper marks` replies and compared with the committed `.paper-text`, `.paper-marks`; `CALCU59_PAPER_REPLIES=1` writes them; LIB-8 AC-39, M-39) |
| `reference_runs_tests.cpp` | 1, Longer (`CalcU59ReferenceRuns`): returns at once without `CALCU59_REFERENCE_COMPARE` or `CALCU59_REFERENCE_RECORD` (the latter writes to `build/reference-runs/`, never into the references; M-34) |
| `rom_lookup_tests.cpp` | 15 (LIB-12 AC-1, AC-4, AC-6, AC-8–10, AC-14, EC-2; M-24) |
| `part_flows_tests.cpp` | 21 (M-55: the card cases moved to `card_flows_tests.cpp`; M-56: the dropped-card cases through LIB-13's `open`, renamed `lib13_ac12_*`, `lib13_ec4_*`, `lib13_ac16_*`; `drop_action_goes_by_name_and_model` went with `dropActionFor`, `part_asm_overlay` to `debug_panel_tests.cpp`; M-54: on format-2 presets and LIB-18's commands; the preset-card tests went with 2.3.0's reader, D-47, for C59-7 in M-55; M-53: LIB-8 AC-8, AC-22, AC-29; LIB-10 AC-30, AC-35 on the run control): the product's flows on the app session (`SessionRig.hpp`, `PartRig`), ported in M-34 from 2.3.0's controller tests (C59-6 AC-1, AC-6, AC-11: 4 of them, M-25); the runner's three pacing tests went with `CooperativePacing` |
| `magnetic_card_tests.cpp` | 19 (C59-7 T1 to T4, M-55; LIB-7 AC-49, release prep step 2: magnetic card format 3 read and written, the bank builder against the ROM-written fixtures `tests/fixtures/cards/*.hex`, the module cue cards of the module files) |
| `card_flows_tests.cpp` | 45 (C59-7 T5 to T9, M-55: the card reader place, fitting, the cue card shown, presets and the harness, 2.3.0's card tests on format 3; LIB-13 AC-12, EC-4 on `.magnetcard.yaml`; `tests/CardRig.hpp`) |
| `media_tests.cpp` | 12 (LIB-7 AC-32 to AC-35, AC-47 of the Calc-U-59 column, M-55) |
| `typing_tests.cpp` | 22 (LIB-5 AC-20–27, T59-0, the printer-timing exit check; M-36) |
| `preset_content_tests.cpp` | 36 (C59-2 AC-1–7, AC-10, AC-13, AC-15–17, EC-1–6: the kind `c59.program-memory`, the sections, the upgrade, the bundle file; M-54) |
| `ti_presets_tests.cpp` | 28 (LIB-6 AC-35–41, AC-53, AC-68, EC-7 on the TI; LIB-5 AC-17, AC-26 in a preset; M-54) |
| `machine_control_tests.cpp` | 17 (LIB-18 AC-16–24 on `TIProductMachines`; ledger row 59-39; M-54) |
| `constant_memory_tests.cpp` | 30 (C59-3 AC-1–14, EC-1–7, LIB-7 AC-49: the kind `c59.constant-memory`, `ti58c.memory.yaml`; M-54) |
| `web_part_tests.cpp` | 6 (LIB-15 AC-16, AC-20, AC-22, EC-4; LIB-9 AC-23; M-34) |
| `debug_model_tests.cpp` | 8 (the panel's cases moved to `debug_panel_tests.cpp` and `asm_overlay_tests.cpp`, M-56) |
| `file_formats_tests.cpp` | 5 (M-55: the 2.3.0 card file cases and their `CARD` records went; `settings_resolve` went with `SettingsKeys.hpp`, M-32; the `STATE` cases went with 2.3.0's reader, M-54) |
| `settings_tests.cpp` | 7 (LIB-9 AC-19–22, AC-24, AC-25, AC-35; M-32) |
| `runtime_tests.cpp` | 6 (`session_plays_keystrokes_in_real_time` went with KEYSTROKES, M-54; `runner_keeps_real_time` went with the threaded runner, M-33; the two batch-rule tests with `Pacing.hpp`, M-34; the freeze and queued-step tests moved to `debugger_tests.cpp` onto the run control, M-53) |
| `data_tables_tests.cpp` | 6 (`pointer_tracker` went with `cu59::PointerTracker`, M-58; LIB-8 AC-30 `lib8_ac30_font_shape`, `lib8_ac30_feed_complete_and_cut_short_heights`, M-53) |
| `pc100c_paper_tests.cpp` | 10 (LIB-8 AC-24–27 and the PC-100C's reference picture, moved from the library; M-53) |
| `trace_tests.cpp` | 6 (LIB-10 AC-33 and the exit check of M-53; replaces `shared_trace_tests.cpp`, M-53) |
| `swift_shell_tests.cpp` | 8 (C59-8 T1–T6, M-58: the menus, the bar, More, the settings rows and AC-19 with the Swift frontend's traits, on the session with LIB-14's `SettingsPart` as the bridge builds it) |

## Calc-U-1600

| CTest entry | What | Group | Measured (M4 Pro, 2026-11-14) |
|---|---|---|---|
| `CoreTests` | about 1000 `TEST`s (M-46: LIB-6 1600 `sharp_presets_tests.cpp`, C1600-4 `preset_content_tests.cpp`, LIB-18 1600 `machine_control_tests.cpp`, the `lib6_` cases of `harness_tests.cpp`; `preset_tests.cpp`, `pc1600_preset_tests.cpp`, `presetloader_trace_tests.cpp`, `app/tests/shared_preset_catalog_tests.cpp` gone; M-44: C1600-2 `program_files_tests.cpp`, `basic_typing_tests.cpp`, `loader_memory_tests.cpp`, `app/tests/loader_part_tests.cpp`, the C1600-2 cases of the loader, machine-code-file and harness tests; M-45: LIB-8 1600 `plotter_paper_tests.cpp`, `paper_session_tests.cpp`, the `lib8_ac37_` cases of `ce150_tests.cpp`, `ce1600p_tests.cpp`; LIB-10 1600 `debugger_tests.cpp`, the `lib10_ac36`/`ac37`/`ac44`/`ac52` cases of `debug_target_tests.cpp`, `run_control_tests.cpp`; C1600-3 `dap_layer_tests.cpp`; the harness's paper and debugger cases; M-39: `tests/paper_replies_tests.cpp`, LIB-8 AC-39; M-36; `tests/typing_tests.cpp`: LIB-5 AC-14, AC-28–31, AC-33–37, AC-40, AC-41; the PC-1500 key queue's tests gone): unit and integration tests of the core and the app layer (real ROMs, presets, loaders, job equivalence), the machine interface (C1600-1, LIB-1 1600, LIB-2 1600), the settings file (LIB-9 1600), the ROM lookup (LIB-12 1600), the golden replays through it (table below), the harness product entry (LIB-11, M-28; the PC-1600 and its variant through LIB-18, `harness_drives_a_pc1600`, `harness_variant_switch_old`, M-35) | Frequent | 39.0 s (M-30/M-28/M-27, 2026-11-15) |
| `CalcU1600SessionTests` | the `TEST`s of the files of `tests/` and `app/tests/` that use `app/tests/SessionFixture.hpp` (a Calc-U-1600 app session and a cold boot per test), split off `CoreTests` by CMake (a file that names `SessionFixture` goes here; Part D simplification P-10) so that ctest runs them side by side; since Part E simplification E-11 those of `tests/` only (122 `TEST`s), those of `app/tests/` are `CalcU1600AppSessionTests`; `CalcU1600DebuggerSpeed` and `CalcU1600PacingRealClock` run cases of this program | Frequent | 21.2 s (E-11, 2026-11-17; 45 s before the split, 31.6 s on 2026-11-16) |
| `CalcU1600AppSessionTests` | the `TEST`s of the files of `app/tests/` that use `SessionFixture.hpp` (61 `TEST`s: `app_session_tests.cpp`, `loader_part_tests.cpp`, `machine_session_tests.cpp`, `media_session_tests.cpp`, `opening_tests.cpp`, `serial_links_tests.cpp`, `shell_product_tests.cpp`), split off `CalcU1600SessionTests` by CMake by their folder (Part E simplification E-11) so that ctest runs the three programs side by side | Frequent | 11.2 s (E-11, 2026-11-17) |
| `calcu_preset_check.CalcU59Presets` | LIB-6 AC-43, C59-2 T9 (M-54): `calcu --product c59 check-preset` over the examples and the three startup presets of `examples/`; since M-55 the four examples with magnetic cards too | Frequent | 0.03 s |
| `CalcU59CalendarScripts` | C59-2 AC-18, T12 (M-54): `tools/calendar/fastcal.py` (`tools/agentic/calendar/` until M-59) regenerates `calendar-09` equal to the tracked file, and `years.py` finds the year in it through `calcu serve` (with Python 3) | Frequent | 2 s |
| `calcu_preset_check.CalcU1600Presets` | LIB-6 AC-51, C1600-4 AC-15 (M-46): `calcu --product c1600 check-preset` over every preset of the product (examples, startup presets, test fixtures, `dev/`, the developer documentation); since M-47 (LIB-7 1600) without exclusions | Frequent | 0.02 s |
| `calcu_preset_check.CalcU1600Templates` | LIB-6 AC-51, C1600-3 AC-19 (M-46): the VS Code extension's templates, filled in as the extension fills them (`tests/fill_vscode_templates.cmake`), checked | Frequent | 0.04 s |
| `CalcU1600MachineIncludeCheck` | `tools/check_machine_includes.sh`: no file outside `core/` (app, frontends, tools, dev) includes a machine class, directly or through a core header, or names one (C1600-1, Technical Requirements; design T13; M-26) | Frequent | 1.1 s |
| `CalcU1600NoOwnRomLookup` | `library/roms/tests/check_no_own_rom_lookup.sh`: no product source outside `core/Machine/SharpRoms.cpp` (with the host-drive ROM, D-6), the catalogue and `app/tests/` looks for a ROM file (LIB-12 AC-2, design T7; M-27) | Frequent | 0.5 s |
| `CalcU1600NoOldTools` | `tools/check_no_old_tools.cmake` (`cmake -P`): the CLIs, the probes, `build_tool.sh`, the one-time checks `dev/tape-matrix/` and `dev/loader-matrix/`, their `.run` configurations and targets are gone; no `PC1600_POWER_PROBE` and no `LegacyCalls.hpp` in the sources (LIB-11 Part D, D.6 H4, D.7; M-51). Replaces `CalcU1600CliRomLookup` (gone with `pc1600_cli`; LIB-12 AC-13 stays covered by the frontends' tests and `tests/rom_lookup_tests.cpp`, the harness's end at a missing ROM by LIB-11 EC-1) | Frequent | 0.02 s |
| `CalcU1600NoPlatformSettings` | `library/settings/check_platform_stores.sh` over `frontends/` with `tools/platform_stores_allowed.txt`: no setting in QSettings or `localStorage` (LIB-9 AC-31, design T-19; M-30); not registered on Windows | Frequent | 0.02 s |
| `CalcU1600ReadTrace` | `tests/read_trace_check.sh`: the harness writes a short PC-1600 trace through the library's trace session, `tools/read_trace.py` reads it (LIB-10 AC-42, design D16-8; M-45); not registered on Windows or without Python | Frequent | 0.3 s |
| `CalcU1600OpenWithTypes` | `tools/check_open_with_types.sh`: the macOS Open With types of the Qt6 app (`Info.plist.in`): the three preset extensions as the product's own, `.bas`, `.bbin`, `.bin` as Alternate, no `.rom` (LIB-13 AC-21, design Part D T16-5; M-48) | Frequent | 0.02 s |
| `CalcU1600NoOwnSession` | `tools/check_no_own_session.cmake` (`cmake -P`): no `AppSession`, `MachineSession` or `Pacer` class in `app/`, the Qt6 frontend includes the library's `sessions/AppSession.hpp`, sends no suspend and runs no operation to its end (LIB-3 AC-34, AC-36; design P16-1, P16-3; M-31) | Frequent | 0.05 s |
| `CalcU1600QtTests.romsMissing` | `CalcU1600QtTests lib14_ac19` with `CALCU1600_QT_ROMS_MISSING=1`: the window over a ROM folder without one file shows the text in place of the faceplate, no bar, no Machine menu; Help and About work (LIB-14 AC-19, ledger 1600-36; M-49), offscreen, Qt trees only | Frequent | 0.8 s |
| `CalcU1600ShotsThroughAttach` | `frontends/qt6/tests/check_shots_through_attach.cmake` (`cmake -P`): `ShotRunner.cpp` names no `controller()`, `pacer()`, `setFrozen`, `runFor`, `pasteText` (LIB-14 AC-44, D.8 Q12; M-49), Qt trees only | Frequent | 0.01 s |
| | **Frequent, Calc-U-1600** | | **40.4 s** (M-30); 32.6 s wall alone with `-j8`, `CoreTests` 26.5 s the longest program, `CalcU1600SessionTests` 21.2 s, `CalcU1600AppSessionTests` 11.2 s; both products together 30.9 s wall with `-j8` (45 to 46 s before E-11 on a quiet machine; 2026-11-17, M4 Pro under other load: the second run 50.7 s, before E-11 75.7 s and 89.3 s); `CoreTests` alone 47 s in M-45, 51 s in M-46, 54 s in M-47, 58.7 s in M-51 (Apple M4 Pro, RelWithDebInfo; M-51 took `CalcU1600CliRomLookup`, 0.8 s, out of the group) |
| `CalcU1600WebPlaywright` | `frontends/web/run-tests.sh`: Playwright, Chromium (root, sub-path) and WebKit (offline; the servers are `library/web/tools/serve.py`, the offline server and the prerender sandbox `library/web/tests/dist-helpers.js`, Part E, E-6); M-46: `lib6_ac49`, `lib6_ac54` (bundled and uploaded), `lib10_ac40 trace step fails on the web`, `lib18_ac32 cold boot menu item`, the startup preset setting; `typing.spec.js` (LIB-5 AC-29, AC-33, AC-36; M-36); M-48: `play.spec.js` `LIB-13 AC-18 …` (2), `LIB-13 AC-19 …`, `LIB-13 AC-20 …`, `LIB-13 AC-8 …`; `play.spec.js` `lib8_ac36 …`, `lib8_ac38 …` (was `a plotter preset draws on the paper`), `lib10_ac40 …`, `lib10_ac41 …` (M-45) | Longer | 55 passed, 2 skipped (M-36); 22.5 s (Homebrew Emscripten 7.0, after `<cstdlib>` was added to `core/Debug/BreakpointTable.cpp`); 31 s in CI (6.0.13) |
| `CalcU1600RomMissingBuild` | `tests/rom_missing_build.sh`: a Web `dist/` built from `roms/` without one file (default `PC1600-P1-B3-old.bin`) shows the lookup's text naming it and no calculator (LIB-12, Technical Requirements, design Q5; M-27); not registered on Windows | Longer | 3.2 s (M-27; the Web build tree already built) |
| `CalcU1600WasmSmoke` | `tools/build_wasm_check.sh`: core and app layer under node | Longer | 5.1 s locally without the build; 44 s in CI with the build |
| `CalcU1600ExampleReferences` | C1600-4 AC-16 (M-46): `CoreTests c1600_4_ac16_examples_match_their_references` with `CALCU1600_REFERENCES=1`: each bundled example loaded through the library's loader and compared with its M-4 reference (display, BASIC and machine-code areas, paper); `Calculat`, `ce150_demo` (paper prefix), biorhythmus' cursor row and DiskWorks' display (M-47 finding) excepted; `make_diskworks_media` also compares the module and disk its `saveas:` writes with those of `examples/dwx/` (M-47) | Longer | 112 s (all 32 compared, M-47) |
| `CalcU1600QtTests` | `frontends/qt6/tests/` (M-49): 22 `TEST`s on the real window of Calc-U-1600 put together from the shell, on the real ROMs (a bundle folder and home of its own) and the real clock, offscreen, the file dialogs, browser and clipboard replaced: LIB-14 AC-34 to AC-41, AC-24, AC-43, AC-44 (an in-process scenario through attach), LIB-4 AC-20, C1600-2 AC-15, LIB-13 AC-19 and AC-21 (a drop, a QFileOpenEvent as the Dock and Finder's Open With send it, a `.bin` opened, the start argument); the design's Frequent did not fit (it waits on the real clock), so Longer | Longer | 24.5 s |
| `CalcU1600Shots.<scenario>` | the screenshot scenarios of `docs/developer/screenshots/guide/`, one per help chapter with pictures since M-52 (`01-introduction`, `02-getting-started`, `04-media`, `05-peripherals`, `06-presets`, `07-debugger`, `08-settings`, `09-product-topics`), run as checks, into the build tree, through harness attach since M-49 (`run:` is wall time: `09-product-topics`, before M-52 `04-basic`, about 90 s) (the media and presets scenarios pass since M-47; the `method: system` shots need the Screen Recording and Automation grants); the check scenarios of `tests/gui/` (M-50: `c1600-3-ac18`, C1600-3 AC-18, AC-21: Settings ▸ Debugger's status line and Disconnect with a client of the process, the app started with `--dap 32177`, 3.6 s; 2026-11-16: `lib5-ac33`, LIB-5 AC-33, the design's `CalcU1600QtTyping`: a host SHIFT tap is one SHIFT press, three shots, offscreen; M-46: `lib6-ac54`; M-45): `lib8-ac36`, `lib8-ac38` (LIB-8 AC-36, AC-38), `lib10-ac41` (LIB-10 AC-41); M-47: `lib7-ac16-module-template-saved`, `lib7-ac8-picker-lists-instance` (LIB-7 AC-45) | Longer | 39.6 s, the nine together (2.2–9.2 s each), all passed; the three of M-45 2.8, 3.6 and 1.7 s; `lib5-ac33` 2.4 s |
| `CalcU1600DapSmoke` | `tools/dap_smoke.py --harness $<TARGET_FILE:calcu>` (M-50, C1600-3 AC-20, AC-22, T14): the ten scenarios against `calcu --product c1600 serve --dap 32170`, no window, each printed as `c1600_3_ac20_<scenario>` (the RENUM one on the fixture ROM of `tests/fixtures/dap/pc1500-bus-rom/`, no skip), and M-50's exit check `m50_exit_vscode_session` (launch, breakpoints, stop, step, variables, continue, as VS Code sends them); where attach is built (`CALCU_ATTACH`), not on Windows | Longer | about 6 s of CPU, 17 s with the scenarios' pauses by hand (M-50, 2026-11-16): all passed; the stepIn finding of 2026-11-14 did not come back |
| `CalcU1600DapSmokeQt` | `tools/dap_smoke.py --app` (C1600-3 T15): the same against the Qt app (`--dap 32169`), Qt trees only | Longer | 33.6 s, all passed (M-50) |
| `CalcU1600HarnessTests` | `tests/harness/` (M-51): 31 `TEST`s, each on a `calcu --product c1600` run of its own (`HarnessRun.hpp`), the replies written out. `product_commands_tests.cpp` (26): C1600-5 AC-1–9, AC-11, AC-12, EC-2–6 (`c1600_5_…`: `dump-basic` against the CLI's lines at the pin, every INSPECTOR view per model, `tape record`/`status` with CSAVE on a CE-150, the CE-158 echo program (`tests/fixtures/basic/ce158_echo.bas`) and `examples:interfaces/setcom1600.pc1600` for `serial`, `host-drive` with `FILES "S3:"`, `call` on the LH5801, the Z-80 and the LH5803); `harness_entry_tests.cpp` (5): LIB-11 AC-18 (`lib11_ac18_every_function_of_the_table`, M-51's exit check), AC-19 (`display-text` with the cursor cell, both models), AC-20 (the entry through `SharpAppPart`). A program of its own so `CoreTests`' budget stays; Longer: `CoreTests` takes 58.7 s of the 60 s of Frequent | Longer | 3.6 s RelWithDebInfo, 10.2 s Debug at `-O1` (Apple M4 Pro, 2026-11-16) |
| `CalcU1600MachineStateTests` | `tests/machine_state_tests.cpp` (C1600-5 T8, AC-13; M-51, second part): 5 `TEST`s on `calcu --product c1600` runs (`tests/harness/HarnessRun.hpp`): `state-save` while a program runs, 2 s, `state-restore` in the same run and in another; the replies, the sound and the state after the 2 s equal; the PC-1500, the PC-1500A (also with a CE-150, CE-158 and module), the PC-1600 (also with the CE-1600P, its disk and tape, two modules and the host drive; and with the CE-150 and CE-158); the refusals for another set of peripherals. A program of its own so `CoreTests`' budget stays | Longer | 12.4 s Debug at `-O1` (Apple M4 Pro, 2026-11-16) |
| `CalcU1600MachineDriver` | `tools/machine/` (2026-11-17): the pytest driver on `calcu serve` (`machine.py`, `cardimage.py`, `library/harness/python/calcu_machine.py`), its 17 cases in `test_driver.py`; pytest 8.3.5 and pytest-xdist through `uv run --with` (fetched once: Longer); registered where uv is | Longer | 1.1 s (M4 Pro, 2026-11-17) |
| `CalcU1600DapTests` | `tests/dap/dap_tests.cpp` (M-50, C1600-3 T3 to T12): the DAP server of `app/dap/` through a test transport in a session of `app/tests/SessionFixture.hpp` (JSON and framing; listening, status, second client; the set-up, Build & Load, held messages, ending, a new machine, the requests, events, own requests); a program of its own, so that `CoreTests`' budget stays | Frequent | 1.4 s (RelWithDebInfo, M-50) |
| `CalcU1600HelpTests` | `tests/help/help_tests.cpp` (LIB-16 L5, P4; M-52): `help/settings-keys.txt` matches `settingsDecl()` (`CALCU1600_HELP_KEYS=1` writes it anew), every declared key is in it, and `help/help.yaml`'s `names:` cover the catalogue's models and peripherals with the bundled module definitions; a program of its own, so that `CoreTests`' budget stays | Frequent | 0.01 s (M-52) |
| `CalcU1600HelpCheck` | `tools/help check calc-u-1600 --quiet-warnings` (LIB-16 AC-1 to AC-5, AC-7, AC-11 to AC-15, EC-1 to EC-3: the checks C1 to C9 over `library/help/` and `help/`); registered when Python 3 is found (M-52), with `CalcU1600HelpBuild` by the root's `calcu_add_help_tests` (Part E, E-6) | Frequent | 0.2 s (M-52) |
| `CalcU1600HelpBuild` | `tools/help build calc-u-1600 --quiet-warnings --out <tree>/products/calc-u-1600/help`: the help asset `Calc-U-1600-help.zip` (LIB-16 AC-6, AC-8: the test suite builds the asset) (M-52) | Frequent | 0.3 s (M-52) |
| `CalcU1600DapPortCheck` | `tests/dap/dap_port_check.sh` (C1600-3 EC-1, D-6): `calcu … serve --dap 80` ends with exit status 2 and the reason; `--dap` outside `serve` too; not on Windows | Frequent | 0.03 s |
| `CalcU1600DebuggerSpeed` | `CoreTests lib10_speed_pc1600_full_speed_with_a_breakpoint` with `CALCU1600_SPEED=1`: a PC-1600 for 20 s of emulated time at full speed, with and without a breakpoint, printed (LIB-10 R-1; M-45) | Longer | 0.5 s (0.22 s and 0.20 s measured) |
| `CalcU1600PacingRealClock` | `lib17_ac21_pacing_longrun_real_clock_pc1500a`, `_pc1600` with `CALCU1600_PACING_LONGRUN=600`, on the library's app session (M-31) | Checkpoint | 20 min (by design) |
| `CalcU1600QtPacingProbe` | `CalcU1600Qt --pacing-probe 60` (an attach client in CALCU_ATTACH builds since M-49): a PC-1600 keeps one minute in the Qt6 frontend (LIB-17 AC-22, P16-3; M-31). The Web half: `frontends/web/tests/session.spec.js` `lib17_ac22` with `CALCU1600_WEB_PACING=1` | Checkpoint | 1 min each; run once in M-31: Qt6 −8 ms, Web −2 ms over 60 s |
| `CalcU1600HelpPictures` | `tools/make_screenshots.sh`: the help's pictures into `help/images/` through harness attach, one scenario per help chapter (LIB-16 AC-12, P2; `CalcU1600GuideScreenshots` before M-52) | Checkpoint | not measured |

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
| `app/tests/opening_tests.cpp` | 20 (LIB-13 AC-17–22, EC-2, EC-7 on the product's open part; M-48) | | |
| `battery_card_instance_tests.cpp` | 19 | `pc1600_ce150_tests.cpp` | 5 |
| `ce150_tests.cpp` | 9 | `pc1600_host_drive_tests.cpp` | 7 |
| `ce151_tests.cpp` | 3 | `pc1600_keyboard_display_tests.cpp` | 38 |
| `ce155_tests.cpp` | 3 | `pc1600_machine_tests.cpp` | 32 (M-51: the yield hook's test gone with the hook) |
| `ce158_tests.cpp` | 17 | `pc1600_phase54_tests.cpp` | 3 |
| `ce1600f_tests.cpp` | 23 (Part D P-4: the floppy resolver's case went with it) | `pc1600_preset_tests.cpp` | 39 |
| `ce1600p_tests.cpp` | 5 | `pc1600_program_placement_tests.cpp` | 12 |
| `ce161_tests.cpp` | 4 | `pc1600_slot_module_tests.cpp` | 13 |
| `ce1638_tests.cpp` | 2 | `pc1600_slot_ram_tests.cpp` | 5 |
| `ce163f_tests.cpp` | 12 | `pc1600_tape_tests.cpp` | 3 |
| `ce502b_tests.cpp` | 2 | `peripherals_tests.cpp` | 8 (LIB-2 AC-28–32, AC-34; M-26) |
| `connector_tests.cpp` | 14 | `piezo_sampler_tests.cpp` | 12 |
| `debug_target_tests.cpp` | 31 (M-45: today's cases as `lib10_ac44_…` on `machine::Debugging`; `lib10_ac36_`, `ac37_`, `ac52_`; the expression cases moved to `dap_layer_tests.cpp`) | `preset_tests.cpp` | 44 |
| `descriptor_tests.cpp` | 9 (LIB-1 AC-34–41; M-26; M-47: `lib1_ac40_one_media_place_capability_per_media_place`) | `presetloader_trace_tests.cpp` | 7 (the core's file mode for today's tools, `lib10_ac42_tools_…`; M-45) |
| `disasm_tests.cpp` | 15 | `program_file_tests.cpp` | 10 (`test_drop_targets` moved to `app/tests/opening_tests.cpp`, M-48) |
| `golden_runs_tests.cpp` | 1 (the golden replays through the machine interface; `CALCU1600_GOLDEN_RECORD=1` records new references only) | `run_control_tests.cpp` | 2 (`lib10_ac44_step_over_a_call`, `_data_breakpoint_and_pause` on the library's run control; M-45) |
| `harness_tests.cpp` | 16 (`harness_drives_a_sharp_machine`: the exit check of M-28, LIB-11 T14, D-1; LIB-18's model and variant; C1600-2 AC-21, the `c1600` loader commands, M-44; LIB-13 AC-18, `lib13_ac18_harness_open_…` (3), M-48; M-45: `lib8_ac32_harness_…`, `lib10_ac40_harness_…`, `lib10_ac52_harness_peek_bank_and_vbank`; `SKIP` without the ROMs) | `debugger_tests.cpp` | 19 (LIB-10 1600: AC-38, AC-39, AC-40, AC-42, AC-43, AC-23 and R-1; M-45) |
| `host_directory_drive_tests.cpp` | 25 | `sc7852_tests.cpp` | 39 |
| `inspector_tests.cpp` | 13 | `sharp_machine_tests.cpp` | 28 (C1600-1 AC-1–21, EC-1–3; M-26) |
| `job_tests.cpp` | gone (Part D P-4: the library's `operation_tests.cpp` covers Task and Job) | `tape_deck_tests.cpp` | 11 |
| `session_tests.cpp` | 4 (LIB-3 AC-35, AC-37, AC-38, ledger 1600-9 on the library's app session; M-31) | `dap_layer_tests.cpp` | 14 (C1600-3 AC-12, AC-14, AC-23: the DAP's layer in `app/dap/`; M-45) |
| `key_paste_tests.cpp` | 8 (the typing profile and engine; M-36) | `tc8576f_tests.cpp` | 20 |
| `lcd_screenshot_tests.cpp` | 5 | `app/tests/serial_links_tests.cpp` | 2 (LIB-2 AC-33; M-26) |
| `lcd_text_tests.cpp` | 21 | `app/tests/app_session_tests.cpp` | 11 (SharpAppPart on the library's session, M-31; the loader cases moved to `loader_part_tests.cpp`, M-44) |
| `lh5801_tests.cpp` | 54 (the key queue's three went, M-36) | `app/tests/media_session_tests.cpp` | 13 (LIB-7 1600 AC-38 to AC-40, AC-44: the bays' tests ported as `lib7_ac44_…`; was `shared_bay_tests.cpp`; M-47) |
| `lh5803_tests.cpp` | 8 | `app/tests/geometry_tests.cpp` | 2 (the plotter paper's three moved to `plotter_paper_tests.cpp` as `lib8_ac37_…`, M-45) |
| `listing_tests.cpp` | 10 | `app/tests/keyboard_map_tests.cpp` | 3 (`SharpKeyboardMap`; M-36) |
| `machine_code_file_tests.cpp` | 39 (C1600-2 AC-8–12, AC-23; M-44) | `app/tests/machine_session_tests.cpp` | 7 |
| `memory_card_tests.cpp` | 45 (LIB-7 AC-48 since release prep step 2; the 8 YAML reader cases moved to the library in M-24; Part D P-4: the 3 resolver cases went with `resolveModuleSpecByName`) | `app/tests/pacing_profiles_tests.cpp` | 2 (LIB-17 AC-20; the sink side of the sound; M-31) |
| `pacing_longrun_tests.cpp` | 4, the "after" test of LIB-17 AC-21 (M-31): `lib17_ac21_pacing_longrun_scripted_pc1500a`, `_pc1600` Frequent; `lib17_ac21_pacing_longrun_real_clock_pc1500a`, `_pc1600` Checkpoint (return at once without `CALCU1600_PACING_LONGRUN`) | `app/tests/shared_preset_catalog_tests.cpp` | 4 |
| `pc1500_tape_tests.cpp` | 2 | `app/tests/settings_tests.cpp` | 8 (LIB-9 AC-26–32; M-30) |
| `rom_lookup_tests.cpp` | 9 (LIB-12 AC-11–14, EC-1; M-27) | `typing_tests.cpp` | 15 (LIB-5, M-36) |
| `paper_replies_tests.cpp` | 1 (`lib8_ac39_sharp_plots_turned_into_paper_replies`: the `.paper` files of M-8 turned into `paper marks` replies, compared with the committed `*.<plotter>.paper-marks`; `CALCU1600_PAPER_REPLIES=1` writes them; LIB-8 AC-39, M-39) | `plotter_paper_tests.cpp` | 10 (LIB-8 AC-32–34, AC-37; M-45; AC-16: the plotters' reference pictures, from the library in Part D) |
| `paper_session_tests.cpp` | 6 (LIB-8 AC-35, AC-36 in the app session; M-45) | `media_kinds_tests.cpp` | 11 (LIB-7 1600 AC-37, AC-41, AC-42, AC-48, AC-49: the codecs, the cassette place, a cassette through the harness; M-47, release prep step 2) |

## Not in a group

Tools and one-time checks, not tests that run again (the inventories,
`docs/inventory/calc-u-59.md` and `calc-u-1600.md`, "Tests"):

- The harness `calcu` itself (`library/harness/cli/`; LIB-11): a tool, tested
  through `calcu_test` and the product entries above.
- Calc-U-59: the goldens generator `tools/goldens/` (regenerated when a
  driver changes), `tools/headless/`, `tools/agentic/` (harnesses),
  `tools/check_parity.sh` (a branch check), Qt `--grab` (by hand).
- Calc-U-1600: `dev/hardware-checks/` (programs for real units).
