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
the CI jobs and `tools/run_tests.sh` need no filter. A tree with both
options and the Qt apps:

```sh
cmake -S . -B build/groups -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCALCU_TESTS_LONGER=ON -DCALCU_TESTS_CHECKPOINT=ON
cmake --build build/groups
ctest --test-dir build/groups -L longer        # or -L frequent, -L checkpoint
ctest --test-dir build/groups/products/calc-u-1600 -L longer   # one product
```

Prerequisites outside the tree: the Playwright entries test a built
`frontends/web/dist/` (`frontends/web/build.sh`); the WASM smoke builds
`build/web` with Emscripten itself; the XCUITest entries need the
simulators of `bin/simulators.yaml` (`bin/simulators setup`); the
Calc-U-1600 screenshot and DAP entries need the Qt app in the tree.

A test that outgrows its group's budget moves up a group or is split
(ADR 0031).

Longer, measured locally on 2026-11-14 at the end of Part B: 291.5 s,
within the 10 min; all entries ran (the two Calc-U-1600 web entries since
the `<cstdlib>` fix).

## Library

| CTest entry | What | Group | Measured (M4 Pro, 2026-11-14) |
|---|---|---|---|
| `calcu_library_tests` | 292 `TEST`s: the library's own tests with the machines built for the tests (`library/machine/tests/TestMachines.*`) and the product declared for the settings tests (`library/settings/tests/TestProduct.hpp`), no product code (LIB-1 AC-24; LIB-1 design §2a); also run alone by the CI job `library` of both product workflows. The pacing tests run on a scripted wall clock (`library/pacing/tests/TestSliceRunner.hpp`), never the host's; so do the session tests (`library/sessions/tests/ScriptedClock.hpp`), with the test frontend and the test product part of `library/sessions/tests/`. LIB-2's host links are tested here with an injected link factory (`peripherals/tests/host_links_tests.cpp`), on every platform; the ROM lookup's tests write their ROM files into a temporary folder (`library/roms/tests/TestRomFiles.hpp`); the harness tests run the product made for them (`library/harness/tests/TestProduct.*`: `test-seg` offering the machine state, `test-dot`) in virtual time, each run in a temporary run folder | Frequent | 0.64 s (M-28, 2026-11-15, RelWithDebInfo; most of it the five-hour scripted runs of LIB-17) |
| `calcu_library_pty_tests` | 4 `TEST`s of `library/peripherals/tests/pty_tests.cpp`: LIB-2's host links on real pseudo-terminals in a temporary folder for serial links (AC-18 to AC-20, EC-7); a program of its own (with the test machines), registered only where `CALCU_HAS_PTY` is set (macOS, Linux), so that it can move to Longer without a design change (LIB-2 design §8) | Frequent | 0.8 s (M-23, 2026-11-15) |
| `calcu_cli.<case>` | LIB-11 T13: `calcu_test` (the harness with the test product only) in a process of its own, run by `library/harness/tests/cli/cli_cases.cmake` with `cmake -P`: argv, standard input and output, exit status; six entries, `ac1_product_by_name_or_short_form_any_case`, `ac2_one_shot_runs_in_order_and_exits_zero`, `ac2_first_refusal_exits_nonzero_with_reason`, `ac3_serve_one_reply_per_line_until_quit`, `ec3_serve_ends_at_end_of_input`, `ec4_unknown_product_lists_products` (M-28) | Frequent | 0.1 s, the six together |
| `calcu_serve_python` | LIB-11 T15: `library/harness/python/tests/test_calcu_serve.py`, the Python helper for `calcu serve` against `calcu_test` (with `uv run --no-project python -I`, else Python 3; M-28) | Frequent | 0.05 s |
| `LibraryPlatformStoreCheck` | `library/settings/tests/check_platform_stores_test.sh`: the self-test of `library/settings/check_platform_stores.sh`, the source check the products run over their frontends (LIB-9 design T-14); not registered on Windows | Frequent | 0.03 s |

`calcu_library_tests` by file (`library/<area>/tests/`; every `TEST` is
Frequent):

| File | `TEST`s |
|---|---|
| `machine/tests/attach_tests.cpp` | 18 (LIB-2) |
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
| `sessions/tests/command_tests.cpp` | 9 |
| `sessions/tests/host_call_tests.cpp` | 6 |
| `sessions/tests/host_service_tests.cpp` | 4 |
| `sessions/tests/library_topics_tests.cpp` | 3 |
| `sessions/tests/mirror_tests.cpp` | 1 |
| `sessions/tests/operation_tests.cpp` | 14 |
| `sessions/tests/sound_tests.cpp` | 5 |
| `sessions/tests/speed_tests.cpp` | 6 |
| `sessions/tests/suspend_tests.cpp` | 5 |
| `sessions/tests/topic_tests.cpp` | 9 |
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
| `formats/tests/yaml_tests.cpp` | 9 (the YAML reader, moved from Calc-U-1600 in M-24; no AC) |
| `formats/tests/formats_tests.cpp` | 3 (LIB-11 AC-12, AC-22, EC-2; M-28) |
| `harness/tests/protocol_tests.cpp` | 8 (LIB-11 AC-2, AC-4, AC-5, AC-13, AC-22; M-28) |
| `harness/tests/commands_tests.cpp` | 13 (LIB-11 AC-5, AC-7, AC-9, AC-11, EC-2, EC-5, EC-7; M-28) |
| `harness/tests/display_text_tests.cpp` | 10 (LIB-11 AC-9, AC-10, AC-23; M-28) |
| `harness/tests/run_tests.cpp` | 18 (LIB-11 AC-6, AC-8, AC-9, AC-21, AC-24, EC-1, EC-3, EC-6; M-28) |

## Calc-U-59

| CTest entry | What | Group | Measured (M4 Pro, 2026-11-14) |
|---|---|---|---|
| `CalcU59Tests` | 229 `TEST`s: unit and ROM-driven integration tests of the core and the app layer, the machine interface (C59-1, LIB-1 59), the ROM lookup (LIB-12 59), the PC-100C and the module slot (C59-6, LIB-2 59), the golden replays through it (table below), the harness product entry (LIB-11, M-28) | Frequent | 8.6 s (M-28) |
| `CalcU59TraceReader` | `tools/read_trace.py` reads the trace `CalcU59Tests` wrote | Frequent | 16.4 s (Python 3.15; 0.3 s in CI) |
| `CalcU59CoreSeam` | `tests/check_core_seam.sh`: no core-internal header outside `core/` and `tests/`, no machine call in the adapters (C59-1 AC-17; M-19) | Frequent | 0.02 s |
| `CalcU59NoOwnRomLookup` | `library/roms/tests/check_no_own_rom_lookup.sh`: no product source outside `app/ProductRoms.*`, the catalogue and the Qt6 Bundle looks for a ROM file (LIB-12 AC-2, design T7; M-24) | Frequent | 1.2 s |
| `CalcU59HeadlessSmoke` | `tools/headless/ti59.py 5 + 7 =` against the CMake-built `ti59headless` shows 12 (C59-1 T14; M-19; with Python 3) | Frequent | 0.04 s |
| `CalcU59HeadlessRomMissing` | `tests/headless_rom_tests.py`: `ti59.py` on a ROM folder without one file ends naming it, on the full set it starts (LIB-12 AC-7, AC-10; M-24; with Python 3) | Frequent | 0.15 s |
| `CalcU59XcodeProjectCheck` | `frontends/swift/check_xcode_project.sh`: Xcode compiles only Swift and the bridge and links `CalcU59.xcframework` (C59-5, ADR 0013; M-15); the app bundles the 24 ROM files of the full set (LIB-12; M-24) | Frequent | 0.02 s |
| | **Frequent, Calc-U-59** | | **25.4 s** (M-25) |
| `CalcU59WebPlaywright` | `frontends/web/run-tests.sh`: Playwright, Chromium (root, sub-path) and WebKit (offline); `rom-missing.spec.js` (LIB-12 AC-10, EC-3; M-24) | Longer | 78 s (M-24) |
| `CalcU59RomMissingBuild` | `tests/rom_missing_build.sh`: roms/ without one file (default `TMC0583.txt`); the headless library and a Web `dist/` built from it start without a calculator and name the file (LIB-12, Technical Requirements; M-24); not registered on Windows | Longer | 3 s (M-24; the Web build tree already built) |
| `CalcU59UIRegression` | XCUITest plan `UIRegressionTests` on the `iphone` simulator | Longer | 154.5 s (iPhone 18 Pro Max, iOS 27.2) |
| `CalcU59PacingRealClock` | `pacing_longrun_real_clock_ti` with `CALCU59_PACING_LONGRUN=600` | Checkpoint | 10 min (by design) |
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
| `pacing_longrun_tests.cpp` | 2: `pacing_longrun_scripted_ti` Frequent; `pacing_longrun_real_clock_ti` Checkpoint (returns at once without `CALCU59_PACING_LONGRUN`) |
| `reference_runs_tests.cpp` | 1, a recorder, not a check: returns at once without `CALCU59_REFERENCE_RECORD` |
| `rom_lookup_tests.cpp` | 15 (LIB-12 AC-1, AC-4, AC-6, AC-8–10, AC-14, EC-2; M-24) |
| `shared_controller_tests.cpp` | 17 (C59-6 AC-1, AC-6, AC-11: 4 of them, M-25) |
| `shared_cooperative_tests.cpp` | 42 |
| `shared_debug_tests.cpp` | 14 |
| `shared_formats_tests.cpp` | 21 |
| `shared_preset_stages_tests.cpp` | 7 |
| `shared_runtime_tests.cpp` | 18 |
| `shared_state_applier_tests.cpp` | 8 |
| `shared_tables_tests.cpp` | 8 |
| `shared_trace_tests.cpp` | 2 |

## Calc-U-1600

| CTest entry | What | Group | Measured (M4 Pro, 2026-11-14) |
|---|---|---|---|
| `CoreTests` | 851 `TEST`s: unit and integration tests of the core and the app layer (real ROMs, presets, loaders, job equivalence), the machine interface (C1600-1, LIB-1 1600, LIB-2 1600), the golden replays through it (table below), the harness product entry (LIB-11, M-28) | Frequent | 39.0 s (M-28, 2026-11-15) |
| `CalcU1600CheckPresets` | `tools/check_presets.sh`: every preset in the repository parses | Frequent | 0.3 s |
| `CalcU1600MachineIncludeCheck` | `tools/check_machine_includes.sh`: no file outside `core/` (app, frontends, tools, dev) includes a machine class, directly or through a core header, or names one (C1600-1, Technical Requirements; design T13; M-26) | Frequent | 1.1 s |
| | **Frequent, Calc-U-1600** | | **39.3 s** (M-26) |
| `CalcU1600WebPlaywright` | `frontends/web/run-tests.sh`: Playwright, Chromium (root, sub-path) and WebKit (offline) | Longer | 22.5 s (Homebrew Emscripten 7.0, after `<cstdlib>` was added to `core/Debug/BreakpointTable.cpp`); 31 s in CI (6.0.13) |
| `CalcU1600WasmSmoke` | `tools/build_wasm_check.sh`: core and app layer under node | Longer | 5.1 s locally without the build; 44 s in CI with the build |
| `CalcU1600Shots.<scenario>` | the nine screenshot scenarios of `docs/developer/screenshots/guide/` run as checks, into the build tree | Longer | 39.6 s, the nine together (2.2–9.2 s each), all passed |
| `CalcU1600DapSmoke` | `tools/dap_smoke.py --app`: the DAP requests VS Code uses | Longer | 24.7 s; failed 3/3 earlier on 2026-11-14 and at the pin (no `stopped` event after the first `stepIn`), passed in the Part B run: possibly timing-dependent, open finding |
| `CalcU1600PacingRealClock` | `pacing_longrun_real_clock_pc1500a`, `_pc1600` with `CALCU1600_PACING_LONGRUN=600` | Checkpoint | 20 min (by design) |
| `CalcU1600GuideScreenshots` | `tools/make_screenshots.sh`: the User Guide images into `docs/images/` | Checkpoint | not measured |

`CoreTests` by file (`products/calc-u-1600/tests/`, `app/tests/`; every
`TEST` is Frequent except where the table says otherwise; a `TEST` that
needs ROMs that are missing reports `SKIP`):

| File | `TEST`s | File | `TEST`s |
|---|---|---|---|
| `basic_fastloader_tests.cpp` | 8 | `pc1600_bank_tests.cpp` | 23 |
| `basic_program_source_tests.cpp` | 8 | `pc1600_basicloader_tests.cpp` | 11 |
| `basictyper_tests.cpp` | 8 | `pc1600_basictyper_tests.cpp` | 12 |
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
| `debug_target_tests.cpp` | 22 | `preset_tests.cpp` | 44 |
| `descriptor_tests.cpp` | 8 (LIB-1 AC-34–41; M-26) | `presetloader_trace_tests.cpp` | 7 |
| `disasm_tests.cpp` | 15 | `program_file_tests.cpp` | 11 |
| `golden_runs_tests.cpp` | 1 (the golden replays through the machine interface; `CALCU1600_GOLDEN_RECORD=1` records new references only) | `run_control_tests.cpp` | 6 |
| `harness_tests.cpp` | 1 (`harness_drives_a_sharp_machine`: the exit check of M-28, LIB-11 T14, D-1; `SKIP` without the ROMs) | | |
| `host_directory_drive_tests.cpp` | 25 | `sc7852_tests.cpp` | 39 |
| `inspector_tests.cpp` | 13 | `sharp_machine_tests.cpp` | 28 (C1600-1 AC-1–21, EC-1–3; M-26) |
| `job_tests.cpp` | 2 | `tape_deck_tests.cpp` | 11 |
| `key_paste_tests.cpp` | 13 | `tc8576f_tests.cpp` | 20 |
| `lcd_screenshot_tests.cpp` | 5 | `app/tests/serial_links_tests.cpp` | 2 (LIB-2 AC-33; M-26) |
| `lcd_text_tests.cpp` | 21 | `app/tests/shared_app_session_tests.cpp` | 14 |
| `lh5801_tests.cpp` | 57 | `app/tests/shared_bay_tests.cpp` | 4 |
| `lh5803_tests.cpp` | 8 | `app/tests/shared_geometry_tests.cpp` | 5 |
| `listing_tests.cpp` | 10 | `app/tests/shared_host_key_map_tests.cpp` | 4 |
| `machine_code_file_tests.cpp` | 22 | `app/tests/shared_machine_session_tests.cpp` | 7 |
| `memory_card_tests.cpp` | 47 (the 8 YAML reader cases moved to the library in M-24) | `app/tests/shared_pacing_tests.cpp` | 3 |
| `pacing_longrun_tests.cpp` | 4: `pacing_longrun_scripted_pc1500a`, `_pc1600` Frequent; `pacing_longrun_real_clock_pc1500a`, `_pc1600` Checkpoint (return at once without `CALCU1600_PACING_LONGRUN`) | `app/tests/shared_preset_catalog_tests.cpp` | 4 |
| `pc1500_tape_tests.cpp` | 2 | `app/tests/shared_settings_tests.cpp` | 4 |

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
