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
| `calcu_library_tests` | 176 `TEST`s: the library's own tests with the machines built for the tests (`library/machine/tests/TestMachines.*`) and the product declared for the settings tests (`library/settings/tests/TestProduct.hpp`), no product code (LIB-1 AC-24; LIB-1 design §2a); also run alone by the CI job `library` of both product workflows. The pacing tests run on a scripted wall clock (`library/pacing/tests/TestSliceRunner.hpp`), never the host's; so do the session tests (`library/sessions/tests/ScriptedClock.hpp`), with the test frontend and the test product part of `library/sessions/tests/` | Frequent | 1.5 s (M-22, 2026-11-14; about 1.1 s of it the five-hour scripted runs of LIB-17) |
| `LibraryPlatformStoreCheck` | `library/settings/tests/check_platform_stores_test.sh`: the self-test of `library/settings/check_platform_stores.sh`, the source check the products run over their frontends (LIB-9 design T-14); not registered on Windows | Frequent | 0.03 s |

`calcu_library_tests` by file (`library/<area>/tests/`; every `TEST` is
Frequent):

| File | `TEST`s |
|---|---|
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

## Calc-U-59

| CTest entry | What | Group | Measured (M4 Pro, 2026-11-14) |
|---|---|---|---|
| `CalcU59Tests` | 188 `TEST`s: unit and ROM-driven integration tests of the core and the app layer, the machine interface (C59-1, LIB-1 59), the golden replays through it (table below) | Frequent | 7.5 s (M-19) |
| `CalcU59TraceReader` | `tools/read_trace.py` reads the trace `CalcU59Tests` wrote | Frequent | 16.4 s (Python 3.15; 0.3 s in CI) |
| `CalcU59CoreSeam` | `tests/check_core_seam.sh`: no core-internal header outside `core/` and `tests/`, no machine call in the adapters (C59-1 AC-17; M-19) | Frequent | 0.02 s |
| `CalcU59HeadlessSmoke` | `tools/headless/ti59.py 5 + 7 =` against the CMake-built `ti59headless` shows 12 (C59-1 T14; M-19; with Python 3) | Frequent | 0.04 s |
| `CalcU59XcodeProjectCheck` | `frontends/swift/check_xcode_project.sh`: Xcode compiles only Swift and the bridge and links `CalcU59.xcframework` (C59-5, ADR 0013; M-15) | Frequent | 0.02 s |
| | **Frequent, Calc-U-59** | | **24.0 s** (M-19) |
| `CalcU59WebPlaywright` | `frontends/web/run-tests.sh`: Playwright, Chromium (root, sub-path) and WebKit (offline) | Longer | 77.7 s |
| `CalcU59UIRegression` | XCUITest plan `UIRegressionTests` on the `iphone` simulator | Longer | 154.5 s (iPhone 18 Pro Max, iOS 27.2) |
| `CalcU59PacingRealClock` | `pacing_longrun_real_clock_ti` with `CALCU59_PACING_LONGRUN=600` | Checkpoint | 10 min (by design) |
| `CalcU59AppStoreScreenshots` | `bin/run-screenshots`: the App Store set (the iPhone Duo is opened flat by hand) | Checkpoint | not measured |
| `CalcU59UITestsMac` | XCUITest plan `UIRegressionTests` on macOS | Checkpoint | not measured (needs a signed runner and the Accessibility permission, `docs/inventory/calc-u-59.md`) |

`CalcU59Tests` by file (`products/calc-u-59/tests/`; every `TEST` is
Frequent except where the table says otherwise):

| File | `TEST`s |
|---|---|
| `core_tests.cpp` | 17 (C59-1 AC-5) |
| `descriptor_tests.cpp` | 7 (LIB-1 AC-26–31, AC-33; M-19) |
| `golden_runs_tests.cpp` | 1 (the golden replays through the machine interface, C59-1 AC-4; `CALCU59_GOLDEN_RECORD=1` records a missing reference only) |
| `machine_tests.cpp` | 26 (C59-1 AC-1–3, 6–16, 18, 19, EC-1–4; M-19) |
| `pacing_longrun_tests.cpp` | 2: `pacing_longrun_scripted_ti` Frequent; `pacing_longrun_real_clock_ti` Checkpoint (returns at once without `CALCU59_PACING_LONGRUN`) |
| `reference_runs_tests.cpp` | 1, a recorder, not a check: returns at once without `CALCU59_REFERENCE_RECORD` |
| `shared_controller_tests.cpp` | 14 |
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
| `CoreTests` | 811 `TEST`s: unit and integration tests of the core and the app layer (real ROMs, presets, loaders, job equivalence), the golden replays (table below) | Frequent | 38.7 s |
| `CalcU1600CheckPresets` | `tools/check_presets.sh`: every preset in the repository parses | Frequent | 0.3 s |
| | **Frequent, Calc-U-1600** | | **39.0 s** |
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
| `basic_fastloader_tests.cpp` | 8 | `pc1600_basicloader_tests.cpp` | 11 |
| `basic_program_source_tests.cpp` | 8 | `pc1600_basictyper_tests.cpp` | 12 |
| `basictyper_tests.cpp` | 8 | `pc1600_ce150_tests.cpp` | 5 |
| `battery_card_instance_tests.cpp` | 19 | `pc1600_host_drive_tests.cpp` | 7 |
| `ce150_tests.cpp` | 9 | `pc1600_keyboard_display_tests.cpp` | 38 |
| `ce151_tests.cpp` | 3 | `pc1600_machine_tests.cpp` | 33 |
| `ce155_tests.cpp` | 3 | `pc1600_phase54_tests.cpp` | 3 |
| `ce158_tests.cpp` | 17 | `pc1600_preset_tests.cpp` | 39 |
| `ce1600f_tests.cpp` | 24 | `pc1600_program_placement_tests.cpp` | 12 |
| `ce1600p_tests.cpp` | 5 | `pc1600_slot_module_tests.cpp` | 13 |
| `ce161_tests.cpp` | 4 | `pc1600_slot_ram_tests.cpp` | 5 |
| `ce1638_tests.cpp` | 2 | `pc1600_tape_tests.cpp` | 3 |
| `ce163f_tests.cpp` | 12 | `piezo_sampler_tests.cpp` | 12 |
| `ce502b_tests.cpp` | 2 | `preset_tests.cpp` | 44 |
| `connector_tests.cpp` | 14 | `presetloader_trace_tests.cpp` | 7 |
| `debug_target_tests.cpp` | 22 | `program_file_tests.cpp` | 11 |
| `disasm_tests.cpp` | 15 | `run_control_tests.cpp` | 6 |
| `golden_runs_tests.cpp` | 1 (the golden replays; `CALCU1600_GOLDEN_RECORD=1` records new references only) | `sc7852_tests.cpp` | 39 |
| `host_directory_drive_tests.cpp` | 25 | `tape_deck_tests.cpp` | 11 |
| `inspector_tests.cpp` | 13 | `tc8576f_tests.cpp` | 20 |
| `job_tests.cpp` | 2 | `app/tests/shared_app_session_tests.cpp` | 13 |
| `key_paste_tests.cpp` | 13 | `app/tests/shared_bay_tests.cpp` | 4 |
| `lcd_screenshot_tests.cpp` | 5 | `app/tests/shared_geometry_tests.cpp` | 5 |
| `lcd_text_tests.cpp` | 21 | `app/tests/shared_host_key_map_tests.cpp` | 4 |
| `lh5801_tests.cpp` | 57 | `app/tests/shared_machine_session_tests.cpp` | 7 |
| `lh5803_tests.cpp` | 8 | `app/tests/shared_pacing_tests.cpp` | 3 |
| `listing_tests.cpp` | 10 | `app/tests/shared_preset_catalog_tests.cpp` | 4 |
| `machine_code_file_tests.cpp` | 22 | `app/tests/shared_settings_tests.cpp` | 4 |
| `memory_card_tests.cpp` | 55 | | |
| `pacing_longrun_tests.cpp` | 4: `pacing_longrun_scripted_pc1500a`, `_pc1600` Frequent; `pacing_longrun_real_clock_pc1500a`, `_pc1600` Checkpoint (return at once without `CALCU1600_PACING_LONGRUN`) | | |
| `pc1500_tape_tests.cpp` | 2 | | |
| `pc1600_bank_tests.cpp` | 23 | | |

## Not in a group

Tools and one-time checks, not tests that run again (the inventories,
`docs/inventory/calc-u-59.md` and `calc-u-1600.md`, "Tests"):

- Calc-U-59: the goldens generator `tools/goldens/` (regenerated when a
  driver changes), `tools/headless/`, `tools/agentic/` (harnesses),
  `tools/check_parity.sh` (a branch check), Qt `--grab` (by hand).
- Calc-U-1600: the CLIs `pc1500_cli`, `pc1600_cli` and the probes
  (harnesses and diagnostics), `dev/loader-matrix/`, `dev/tape-matrix/`
  (one-time verifications), `dev/hardware-checks/` (programs for real
  units).
