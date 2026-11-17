# LIB-11: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-28, 2026-11-15, branch `m28-harness`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo (`build/tests`, Qt off), and in the library-only tree
(`-DCALCU_PRODUCT_59=OFF -DCALCU_PRODUCT_1600=OFF`). The code is
`library/harness/` (`calcu::harness`) and `library/formats/Flow.*`,
`Duration.*`; the program `calcu` and its test form `calcu_test` are in
`library/harness/cli/`. The library cases are in `library/harness/tests/`
and `library/formats/tests/formats_tests.cpp` (`calcu_library_tests`); they
run the product made for the tests (`TestProduct.*`: Calc-U-Test, `test`,
models `test-seg` with the machine state and `test-dot`, the test font and
segment rule, the topic `test.count`, the commands `test mapping`,
`test operation`, `test fail`, `test count`). The process cases are CTest
entries `calcu_cli.<case>` (`library/harness/tests/cli/cli_cases.cmake`
with `calcu_test`). The sources of `library/harness/` and the new
`library/formats/` files also compile with Emscripten (`em++ -std=c++20
-fwasm-exceptions -Wall -Wextra`).

Result: `calcu_library_tests` 292 `TEST`s (52 of them LIB-11's), 2,505
checks passed, 0 failed, 0.64 s; `calcu_cli.*` 6 of 6 and
`calcu_serve_python` pass. The whole Frequent group passes (20 of 20 CTest
entries, 39 s wall time with `-j8`; Calc-U-1600's `CoreTests` 41 s).

| ID | Result | Test case |
|---|---|---|
| AC-1 | Pass | `calcu_cli.ac1_product_by_name_or_short_form_any_case` (`Calc-U-Test`, `calc-u-test`, `TEST`, `test`); the products register through `calcu_harness_add_product` and the deferred table (`calcu --product 59`, `--product 1600` in one program) |
| AC-2 | Pass | `ac2_commands_part_on_then` (`protocol_tests.cpp`); `calcu_cli.ac2_one_shot_runs_in_order_and_exits_zero`, `calcu_cli.ac2_first_refusal_exits_nonzero_with_reason` (`wait 1s then test fail then ticks`: exit 1, the reason, `ticks` not run). The clause "the machine the product starts with (LIB-18)" passes in M-35: `ac2_run_starts_with_the_startup_model_under_default_settings` (`run_tests.cpp`; LIB-18's `StartupPart` builds the startup model and the run begins at key readiness); adapter A-1 is replaced by LIB-18's `HarnessMachines` |
| AC-3 | Pass | `calcu_cli.ac3_serve_one_reply_per_line_until_quit` (a `\r\n` line, an empty line, a refused line, `quit`, a line after it) |
| AC-4 | Pass | `ac4_plain_reply_is_lines`, `ac4_json_reply_has_the_three_keys_for_every_command` (every registered command, done and refused, one line each) |
| AC-5 | Pass | `ac5_refusal_carries_the_reason`, `ac5_line_that_is_no_command_is_refused` (and serve goes on), `ac5_notices_only_through_state` |
| AC-6 | Pass | `ac6_two_runs_give_the_same_replies` (18 commands, plain and `--json`, 30 ms of wall time between the runs, byte for byte), `ac6_no_pacer_and_no_wall_clock` (wall time passing moves nothing; the session is hosted in virtual time and its wall clock, which stands at 0, is not read by any command) |
| AC-7 | Pass | `ac7_operation_runs_to_its_end_before_the_reply` (2 s operation; busy idle again before the reply) |
| AC-8 | Pass (the harness side) | `ac8_defaults_and_no_settings_file_read_or_written`, `ac8_run_folder_empty_removed_and_in_no_reply`, `ac8_traces_and_screenshots_go_to_the_run_folder`, `ac8_options_name_settings_save_and_output_folders`, `ac8_bundled_roms_through_the_lookup`. M-38: bundled presets (`ac34_preset_done_or_refused_with_line`, `preset examples:basic/two.test-seg`; `ac34_preset_list`) and screenshots into the run's folder (`ac34_screenshot_into_the_run_folder`, `preset_commands_tests.cpp`). Bundled media and the traces themselves pass in M-41 and M-40 (design §1) |
| AC-9 | Pass | `ac9_wait_reports_time_passed_within_one_instruction`, `ac9_key_down_and_up`, `ac9_unknown_key_is_refused`, `ac9_ticks_gives_count_and_seconds`, `ac9_capabilities_with_kind_id_owner_and_places`, `ac9_quit_ends_serve`, `ac9_lcd_reads_display_topic`, `ac9_audio_records_all_sound_also_of_an_operation`. `wait` ending at once while the debugger pauses passes in M-40 |
| AC-10 | Pass | `ac10_dot_matrix_rows_indicators_and_cells_not_text`, `ac10_inverted_glyph_is_text`, `ac10_segment_positions_left_to_right_by_product_rule`, `ac10_display_off_says_so` |
| AC-11 | Pass (library and product topics) | `ac11_state_reads_library_and_product_topics` (`display`, `busy`, `notices`, `log`, `test.count`), `ac11_stream_gives_what_was_appended_since_last_read`. Other features' topics pass in their steps |
| AC-12 | Pass | `ac12_wait_takes_the_steps_duration_forms` (`formats_tests.cpp`); the bare `wait` until idle, at most an hour, with the time and whether the hour was reached: `ac34_wait_until_idle_reports_time_and_hour` (`preset_commands_tests.cpp`; LIB-6 AC-34, M-38) |
| AC-13 | Pass | `ac13_product_command_under_product_word`, `ac13_common_command_at_top_level` |
| AC-21 | Pass (build, operations, the option) | `ac21_fixed_instant_by_default_at_build_and_after_operation`, `ac21_clock_option_names_instant`, `ac21_clock_host_hands_host_time`; `syncclock` takes the run's clock: `ac34_syncclock_takes_the_run_clock` (`preset_commands_tests.cpp`; LIB-6 AC-27, AC-34, M-38) |
| AC-22 | Pass (with a test product command) | `ac22_flow_mapping_as_in_a_preset` (the three examples of the spec), `ac22_step_becomes_command_by_dropping_dash_and_colon`; the real mapping commands pass in M-41 |
| AC-23 | Pass | `ac23_dot_matrix_rows_of_ones_and_zeros`, `ac23_segment_positions`, `ac23_indicator_bits_in_descriptor_order`, `ac23_display_off`, `ac23_json_arrays` |
| AC-24 | Pass (library; test machine) | `ac24_restore_continues_identically` (same run and another run), `ac24_restore_refused_for_other_configuration_changes_nothing` (another version, another model in the header, not a state), `ac24_refused_without_the_kind`, `ac24_host_files_not_in_state`. The products offer the kind with their own steps |
| EC-1 | Pass | `ec1_missing_rom_ends_at_start_naming_files` (two files named; `calcu` exits 2 with the message on standard error) |
| EC-2 | Pass | `ec2_duration_without_unit_is_refused` (`formats_tests.cpp`), `ec2_wait_without_unit_is_refused` |
| EC-3 | Pass | `calcu_cli.ec3_serve_ends_at_end_of_input` (WAV header completed), `ec3_wav_complete_when_input_ends` (two `audio` files, D-8) |
| EC-4 | Pass | `calcu_cli.ec4_unknown_product_lists_products` (an unknown product and no `--product`, D-5) |
| EC-5 | Pass | `ec5_state_of_no_topic_is_refused` |
| EC-6 | Pass | `ec6_audio_on_machine_without_sound_is_refused`; on a TI-59 in `harness_drives_a_ti59` |
| EC-7 | Pass | `ec7_repeated_key_down_or_up_is_done_and_changes_nothing` |

The Python helper (D-2, no AC): `calcu_serve_python` passes
(`library/harness/python/tests/test_calcu_serve.py`, JSON and plain).

### Exit check of M-28

`calcu` drives a TI-59 and a Sharp machine headless:
`harness_drives_a_ti59` (`CalcU59Tests`; `wait`, `key-down`/`key-up 7`,
`lcd` shows `          7 ` as `ti59.py`'s `display()` does, `ticks`) and
`harness_drives_a_sharp_machine` (`CoreTests`; the PC-1500A of the default
start, D-1: `NEW0? :CHECK` read from the dots with the ROM font, answered
with keys, `display`). The PC-1600 follows in M-35 (model switch, LIB-18):
`harness_drives_a_pc1600` and `harness_variant_switch_old` (`CoreTests`;
`model PC-1600`, `lcd` reads `>` with the PC-1600's font, `variant old`).
By hand: `calcu --product 1600 wait 2s then lcd` shows `NEW0? :CHECK`,
`indicators: DE G PRO I`, `not text: 0`.

### Golden runs

Both products' golden replays pass unchanged in their Frequent suites
(`golden_runs_tests.cpp` of `CalcU59Tests` and `CoreTests`). M-28 touches
no core; no reference was re-recorded.

### Deviations from the design, chosen by the simpler structure

- `HarnessProduct` (§3.6): `lookUpRoms()` instead of `bundle()` (the
  product's own lookup: Calc-U-59 needs its text reader), `startupRequest`
  also takes the `RomSet`, `startUp(Machine&)` (Calc-U-1600's builder runs
  nothing, so A-1 makes the first cold boot), `version()` (for AC-24's
  header), and `sessionParts()` instead of `addToSession(AppSession&)`
  (LIB-3 takes its parts at construction). The Calc-U-1600 font (A-4) is
  read by such a part when the machine changes.
- `Protocol.cpp` is part of `Commands.cpp`; `StatePart.*` and
  `MachineState.hpp` are new files (AC-24, added to the design on
  2026-11-14 without files). The generic kind `machine-state` lives in
  `calcu::harness`, the feature that builds it (LIB-1 design §2a); the test
  machines offer it only with `offerState(true)`, so the other areas' lists
  stay as they are.
- The product entries are targets of their own, `calcu59_harness` and
  `calcu1600_harness` (`app/HarnessProduct.cpp`), not part of
  `calcu59_shared` and `calcu_shared`, so the Swift, Qt6 and Web builds
  carry none of them and no source path is compiled into them.
- `FirstCommands.cpp` reads and writes the files of `audio`, `state-save`
  and `state-restore` itself (`WavRecorder` too); §8 wants file access in
  `Run.cpp` only. Standard C++ only, it compiles for Emscripten and iOS.
- Calc-U-1600's ROMs: until M-27 the entry runs the library's lookup over
  the folders the tests use (`roms/`, `firmware/pc1600-hostdrive/`) and the
  catalogue without module definitions for the full set.
- After `key-down` the display topic shows the change at the next `wait`
  (LIB-3 reads the display when time passes), as in today's frontends.
- The first read of a stream topic carries no reset marker from the
  listen's replay.
- `version()` gives the apps' version, read by CMake from the same single
  source the frontends use: Calc-U-59 the Xcode project's
  `MARKETING_VERSION` (3.0.0), Calc-U-1600 the Qt build's `APP_VERSION`
  (0.9.0). User, 2026-11-15 (the M-28 build had the CHANGELOG versions).

### M-35 (LIB-18 Library, 2026-11-15)

- The harness's start is LIB-18's (design §3.6): `Server::start` adds
  `StartupPart` and sends `start`; `HarnessProduct::startupRequest` and
  `startUp` (adapter A-1) are gone, `machines()` gives LIB-18's
  `ProductMachines` (adapter `HarnessMachines` in both product entries,
  with `SharpBuildColdBoot` in Calc-U-1600's; LIB-18 design §5).
- A run now begins at key readiness, not at tick 0 after the build: the
  expected `ticks` and display values of `commands_tests.cpp`,
  `display_text_tests.cpp`, `protocol_tests.cpp`, `run_tests.cpp`,
  `calcu_cli.*` and `calcu_serve_python` moved by the start-up (one run
  piece: 1,667 ticks on test-seg, 16,668 on test-dot; LIB-18 design §8).
- A settings file named with `--settings` gets `[startup] last-model`
  written, as a product writes it (LIB-18 AC-9); without `--settings`
  nothing is written (AC-8). `ac8_options_name_settings_save_and_output_folders`
  checks that only `last-model` was added.
- The harness has LIB-18's commands `model`, `variant`, `attach`,
  `detach`, `reset`, `cold-boot`, `power-cycle` (LIB-18 AC-15).

## Calc-U-1600 (M-51, 2026-11-16, branch `m51-cli-1600`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja: the
worktree's `build/tests` (Debug, the tests at `-O1`, Qt off) and
`build/groups` (RelWithDebInfo, Qt on, Longer on). With C1600-5 (its
qa-report has the commands).

The code (Part D):
- `library/harness/Product.hpp`: `prepare(RunSetup&)` (D-11) and
  `appLayerParts()` (`AppLayerParts`: the library's parts a product's app
  layer brings); `library/harness/Run.*`: `Server::start` calls `prepare`
  first, then, with an app layer, takes its parts (the control part first,
  as the frontends' session builders put it) and builds none of its own;
  without one, as before (Calc-U-59, the test product).
- `HarnessHost::command(…, within)`: a shorter wait for a deferred reply
  (`pc1600 call`'s timeout).
- `products/calc-u-1600/app/HarnessProduct.cpp`: the entry builds
  `Settings`, `AppPaths` (the run's folders, the source tree's bundled
  folders: `BundleDirs::roms`, `modules`, `hostDriveRom`) and `SharpAppPart`
  in `prepare`; its library parts are the run's; `sessionParts()` are the
  app layer's, then `LcdTextPart` and `SerialScriptsPart`. Before a run
  (the CLI's options, harness attach to the Qt6 app) the entry keeps a
  catalogue, `SharpProductMachines` and preset rules of its own.
- `app/LcdTextPart.*` (was the `FontPart`, A-4): the ROM font at a new
  machine, the cursor at each flush (`lcdCursorOf`, `uncoverCursor` in
  `core/Display/DisplayOutputText`).

Tests: `products/calc-u-1600/tests/harness/harness_entry_tests.cpp`, 5
`TEST`s in `CalcU1600HarnessTests` (Longer; see C1600-5); the library's
`prepare_runs_before_session_parts` in `run_tests.cpp`
(`calcu_library_tests`); the CTest `CalcU1600NoOldTools` (Frequent).

| ID | Result | Test case |
|---|---|---|
| AC-18 | Pass | `lib11_ac18_every_function_of_the_table`: one block per row of D.5 on one run (`--preset` → `preset`, `wait`, `expect`; `--check-preset` → `check-preset`; `--rom`, `--ce1600p-rom` → `variant Old`, `attach printer CE-1600P:old`; `--run-after` → `wait 1s`; `--lcd-text` → `lcd`; `--lcd-png` → `screenshot` into `--output-folder`; `--wav` → `audio` (complete at the end of the run); `--dump-mem` → `peek`; `--inspect` → `pc1600 inspect`; `--dump-basic` → `pc1600 dump-basic`; the summary → `registers`, `history`, `profile`; `--tape-in`/`--tape-out` → `pc1600 tape record`, `eject tape`, `tape { name: Empty }`, `pc1600 tape status`; `--save-dir`/`--modules-dir` → `--save-folder` (`saveas` of the tape there) and the bundled module definitions in `media`; `--ce158-*` → `pc1600 serial ce158 feed … hold=2`, `capture`, `report`, `link`). The probes: not taken over, the CTest `CalcU1600NoOldTools`; `read_trace.py`: `CalcU1600ReadTrace`; `dap_smoke.py`: `CalcU1600DapSmoke`; `--shots`: `CalcU1600Shots.*` through attach. With C1600-5's tests every command of the table has its own test too |
| AC-19 | Pass | `lib11_ac19_lcd_rows_indicators_not_text_off` (`NEW0? :CHECK`, the indicators, `not text: 0`; `display off` after the off key; the PC-1600's prompt), `lib11_ac19_cursor_cell_read_as_text` (`ABC` typed, the cursor moved onto the B, sampled every 20 ms for a second on the PC-1500A and the PC-1600: while the ROM reports the cursor, `lcd` reads ABC though the display shows the cursor over the B; when it does not, `lcd` is the display read with the font alone, by the rule `--lcd-text` of the pin used) |
| AC-20 | Pass | `lib11_ac20_entry_runs_sharp_app_part` (SharpAppPart's topics `c1600.serial-links`, `c1600.lamps`; LIB-4's `control-state` with the product's rules; LIB-7's `media`; the product command `c1600.mount-host-drive` through `pc1600 host-drive`), `lib11_ac20_roms_and_module_definitions_as_the_frontends` (the bundled modules, disks and the tape template; the ROM set's CE-502B, attached); `prepare_runs_before_session_parts` (library); every test of `CalcU1600HarnessTests` and `CoreTests`' `harness_tests.cpp` runs on this entry |

Coverage of the Calc-U-1600 column: AC-18 to AC-20, 3 of 3. Column Done.

### Exit check of M-51

Every function of the table of AC-18 through `calcu`:
`lib11_ac18_every_function_of_the_table` and C1600-5's tests pass.

### Deviations from the design, chosen by the simpler structure

- **The library's parts come from the app layer** (spec decision log).
  D.3 assumed `SharpAppPart::sessionParts()` holds none of the library's
  parts; it holds the peripherals, presets, startup, media and open parts
  (and the control part beside them). The run takes them
  (`appLayerParts()`) instead of building a second set; the paper and
  debugger steps and the media steps are the app layer's. The harness's
  serial port files come only when `link` asks (`SharpAppPartOptions::
  serialLinksOnRequest`; C1600-5 AC-9), typing runs in emulated time
  (`hosting`), screenshots go to the run's output folder.
- **`prepare` takes `RunSetup&`**, not `const`: the app layer writes the
  run's settings (LIB-18 AC-9, `last-model`).
- **The entry's members before a run.** The CLI asks `serveAddOns()` and
  harness attach asks `presetRules()` and `addCommands()` without a run:
  the entry keeps its own catalogue, machines and preset rules for that;
  the DAP add-on reaches the DAP part of the run it starts in.
- **The tests** are the program `CalcU1600HarnessTests` (Longer), not
  `CoreTests` (D.6): `CoreTests` uses nearly all of the Frequent budget.
- **The cursor test** compares `lcd` with the display read with the font
  alone at the same tick, not with rows of `--lcd-text` written out (the
  CLI's ticks and the harness's do not meet); the rule is `--lcd-text`'s
  (`parseLcdText`), now `uncoverCursor`.
- **Removed beyond D.5**, code that only the tools or the preset loader of
  the pin used: the yield hook of both machine classes (and its test), and
  `SharpMachine::syncFromCore`/`coreHasPlugged` (the adapter
  `applyPresetJob`'s); the library hook `ProductMachines::startUp` (adapter
  `SharpBuildColdBoot`, which M-46 retired on the product side).
- **`makeHarnessProduct(romFolder)`** (the CLIs' ROM folder) is gone.

### The adapter check (plan §1; D.7)

Every adapter row of the approved designs about Calc-U-1600 or the
library, removed in M-51 or earlier, looked up in the code (the §5 tables
of LIB-1 to LIB-18, C1600-1 to C1600-5, the Part D sections), and the
source search of D.7 (`LegacyCalls`, `HarnessMachines`,
`SharpBuildColdBoot`, `toDisplayFrame`, `modelIdOf`, `c1600.control-state`,
`c1600.machine`, `c1600.tapes`, `c1600.plotter-paper`, `c1600.trace`,
`c1600.tape-deck`, `c1600.floppy-drive`, `c1600.key-queue`).
`CalcU1600NoOldTools` fails if `LegacyCalls.hpp` or the probes' hooks
come back.

| Row (design) | Removed in | Result |
|---|---|---|
| Model and ROM version enums, `MachineModel.hpp` (LIB-1; C1600-1 `modelIdOf`, `modelOf`, `variantIdOf`) | M-49 | Gone (`modelOf` in `PC1500InterfaceMachine.cpp` is the core's own function from a descriptor to `PC1500Variant`) |
| Sharp peripheral calls, write-protect, product-held host links, clock seeding, `syncclock` (LIB-2) | M-26 to M-47 | Gone |
| `SharpTypingHook`, `SharpDebuggerGate`, interim topics 1600, Sharp operations as product commands (LIB-3) | M-36 to M-49 | Gone |
| `JobForward`, `core/Job.hpp` (LIB-3, LIB-6: "at the latest M-46") | M-46 | **Finding, stays**: `namespace job` is used by the Sharp typers of the core (`PC1500BasicTyper`, `PC1600BasicTyper`) and the core's tests (`BootRun.hpp`, `job_tests.cpp`); no file above the core uses it (M-51 took the last include out of `app/SharpAppPart.hpp`). It goes with the typers (next row). Part D (P-4, `ds-core`): moved to `tests/support/Job.hpp`, out of `calcu_core` |
| Sharp preset typers `tapKeyJob`, `typeLineJob`, `typeBasicProgramTextJob` (LIB-5; C1600-2) | M-44, M-46 | **Finding, stays**: their app and preset users are gone (M-44, M-46; the CLIs' last use went in M-51); they remain the core tests' way to type into a bare machine (25 test files use `typeBasicProgramText`, `typeLine`, `tapKey`). Moving those tests onto LIB-5's typing engine is a step of its own; until then the typers are test support, not an adapter of a caller. For the user at the part boundary. Part D (P-4, `ds-core`): moved to `tests/support/`, compiled into `CoreTests` only; the tests stay on them |
| `c1600.control-state` (LIB-4, LIB-14) | M-49 | Gone |
| A-1 interim Sharp media, A-3 `planMediaSave`, `savePresetMedia` (LIB-7) | M-47 | Gone |
| A-2 `AppPaths` folders (LIB-7), A-1 `AppPaths::instanceDir`, `tapeDir`, `traceDir`, `serialLinkDir` (LIB-9) | M-47 | **Stays, as M-47 recorded** (LIB-7 qa-report: the Settings dialog and LIB-9's tests read them): thin helpers over `calcu::settings::Paths` that make the folder; the old callers they wrapped (the bays, `AppSession`, `MachineSession`) are gone. No old call is left behind them |
| `c1600.plotter-paper`, `PlotterPaper`, the paper panel widgets (LIB-8; C1600-1) | M-45 | Gone (`PlotterPaperOutput` is LIB-8's paper output) |
| A-3, A-4 Web trace and screenshot folders (LIB-9) | M-45, M-46 | Gone |
| `SharpDebuggerGate`, `makeDebugTarget`, `c1600.trace`, `DapRunControlBridge`, L-A1 Qt DAP, `loadProgram` (LIB-10; C1600-3) | M-45, M-50 | Gone |
| A-1 `startupRequest`, A-3 minimal `ProductDecl` (LIB-11) | M-35, M-30 | Gone |
| A-4 the font of `lcd` (LIB-11) | M-51 | Gone: `app/LcdTextPart` with the cursor (AC-19) |
| A-5 `MachineSession::Listener::romMissing` (LIB-12) | M-31 | Gone |
| `looksLikePreset` forward, `DropFile.*`, the drop flow (LIB-13; C1600-2) | M-48 | Gone |
| Operations popup, `--pacing-probe` as its own stepper, frozen mode, W16-1 page data, `engine.js`, settings commands of `SharpWebPart`, A-4 drawing (LIB-14, LIB-15) | M-49 | Gone (`--pacing-probe` is an attach client) |
| `HarnessMachines` (LIB-18) | M-46 | Gone (its name in a comment of `library/startup/ProductMachines.hpp` changed) |
| `SharpBuildColdBoot` (LIB-18) | M-46 | The product side went in M-46; the library hook `ProductMachines::startUp`, which nothing overrode, removed in M-51 |
| `SharpRomImages`, `c1600.key-queue`, `LegacyCalls::readLatches`, `loadBasicProgram`, `loadMachineCode`, `machineCodeLoadState`, `userRam`, `makeDebugTarget`, `loadProgram`, `applyPresetJob`, `runToPromptJob` (C1600-1) | M-27 to M-50 | Gone |
| `c1600.tape-deck`, `c1600.floppy-drive` kinds (C1600-1) | M-47 | Stay as **product kinds**, not adapters: M-47 moved the tape and the disk to LIB-7's media places and kept the two kinds for the counter, the motor and the drive's lamp (LIB-7 design §10, qa-report); `SharpProductKinds.hpp` now lists them with the product kinds. C1600-5's `tape status` reads the counter |
| `LegacyCalls::moduleIn`, `atOffPoint` (C1600-1; the LIB-7 qa-report kept them) | M-47 | Gone in M-51: `SharpAppPart::slotModuleName` (the module's name from `plugged()`) for the card dump and the tests; the tests' own `sharptest::moduleIn` and `SharpMachine::atOffPoint` inside `tests/` |
| The CLIs and probes on `LegacyCalls` and the adapter kinds; `coreParts`, `setYieldHook`, `presetMedia`, `typeLine` (C1600-1 §7, D.5) | M-51 | Gone, with `core/Machine/LegacyCalls.*`, `PC1600_POWER_PROBE`, `calcu_core_probe` |
| Calc-U-59's rows of the library (`TIFreezeGate`, `TIKeySequencerHook`, `CooperativePacing`, `SliceRequest::due`, `TICardWaitAdapter`, the `c59.*` interim topics, LIB-2's TI rows, LIB-9 A-2, A-5, A-6, …) | M-53 to M-59 | Listed and left (M-59's check) |

So no adapter of Calc-U-1600 is left in the plan's sense (an old call
kept behind a seam for a caller that has not moved): what remains is the
typers with `JobForward`, as the core tests' support, and `AppPaths`'
folder helpers, each with its reason above.

### Golden runs

`CoreTests` passes unchanged, the golden replays
(`c1600_1_ac4_golden_runs_through_the_interface`) among them; no golden run
differs.

## Calc-U-59 (M-59, 2026-11-17, branch `m59-harness-59`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja: the
worktree's `build/tests` (RelWithDebInfo, Qt off; Frequent), `build/qt`
(the Qt6 app), the XCFramework and the Swift app (`xcodebuild`, macOS). With
C59-9 (its qa-report has the `ti59` commands and the machine state).

The code (§11.2):
- `products/calc-u-59/app/HarnessProduct.cpp`: the entry builds `TIAppPart`
  in `prepare()` on the run's settings and folders (§11.3: the save folder,
  the traces and screenshots into the output folder, typing in emulated
  time, the debug panel's tabs computed as if shown, `TIAppSetup::
  debugTabsShown`); its library parts and session parts are the run's;
  before a run, machines of its own. `CALCU59_ROMS_DIR` names another ROM
  folder (the ROM-missing build). `TIAppPart::operationEnded` now follows a
  machine an operation replaced before it reads the plugs: an operation run
  in one go (the harness) ended before the session followed the new
  machine, and the part read the old one (a use after free on `model`).
- `core/TIAdapters.*` → `core/TITypingSupport.*` (D-17): the typing support
  and `idleAndKeyDetectRange()`; `TIMemoryAdapter` and `memoryOf` are gone
  (the debug log and snapshots read `c59.program-memory`, which got
  `registerTop()`; the core tests `TIMachineTesting`).
- Removed: `tools/headless/` (`capi.cpp`, `ti59.py`, `build.sh`, README),
  `tools/agentic/` (README, `linetimes.py`, `runtime.py`, `hotsteps.py`,
  `stepcost.py`, `kdis.py`), the target `ti59headless`, the CTests
  `CalcU59HeadlessSmoke` and `CalcU59HeadlessRomMissing`,
  `tests/headless_rom_tests.py`. Moved onto `calcu serve` (D-16):
  `tools/coverage.py`, `tools/fastmode_matrix.py`, `tools/calendar/` with
  `asm59.py`, the helper `tools/calcu59.py`; `tools/read_trace.py` got
  `--labels` and `--changes` (C59-9 D-9). New: `reference/Harness.md` (D-18),
  `.run/calcu-59.run.xml`, `tools/check_no_old_tools.cmake`,
  `tests/calcu_runs.cmake`, `tests/harness_rom_tests.py`.

Tests: the `lib11_…` cases of `products/calc-u-59/tests/harness_tests.cpp`
in `CalcU59Tests` (Frequent); the CTests `CalcU59NoOldTools`,
`CalcU59CalcuRuns`, `CalcU59HarnessRomMissing` (Frequent), `CalcU59Scripts`
(Longer). `CalcU59Tests`: 591 `TEST`s, 0 failed; Frequent all passed.

| ID | Result | Test case |
|---|---|---|
| AC-14 | Pass | `lib11_ac14_entry_runs_ti_app_part` (TIAppPart's topic `c59.cue-card`, LIB-7's `media` with the card reader, LIB-4's `control-state`, `ti59 partition`; the model switch followed); `lib11_ac14_every_function_of_the_table`: one block per row of C59-9 §4 on one run (one-shot and `serve`; the ROMs missing named; `model`, `attach slot-1 ML`, `attach printer PC-100C`, `power-cycle`, `reset`, `cold-boot`; `key-down`/`key-up`, `key CLR 2nd pi`; `switch PC-100C TRACE|PRINT|ADV`; `wait <d>`, `wait`, `pause`, `step { level: cpu, count: 10 }`, `resume`; `ticks`; `trace` of keys and of a cold boot; `profile report`; `ti59 coverage`; `registers TMC0501`, `peek … scom`, `disassemble`, `poke`/`peek … ram`; `ti59 program`; `lcd`, `display`; `ti59 line-times`, `paper text`, `paper marks`; `key 1 Write` with `state media` waiting, `card { mem: X }`, `saveas { card: … }`, `arm`; `preset`, `ti59 run-time`, `hot-steps`, `step-trace`, `step-cost` (refused while a program runs, then done); `ti59 keycodes disassemble|list|assemble`, `ti59 registers`, `ti59 partition`); the scripts: `CalcU59Scripts`, `CalcU59CalendarScripts`, `CalcU59TraceReader`; the old tools gone: `CalcU59NoOldTools` |
| AC-15 | Pass (macOS) | `CalcU59CalcuRuns` (`tests/calcu_runs.cmake`, the process test `lib11_ac15_calcu_runs_calc_u_59`); the same script is a step of the `linux` and `windows` jobs of `calc-u-59.yml` after their build: Linux and Windows at the CI dispatch (plan §1: the step changes the workflow) |
| AC-16 | Pass | `lib11_ac16_every_segment_pattern_gives_its_character` (the ten digits, -, °, ", ', a position not driven, a pattern of no glyph; each with and without the point; no hexadecimal digits, D-15), `lib11_ac16_lcd_twelve_positions_with_points` (`        -1.5 `, ` 1234567890 `), `lib11_ac16_indicator_c_line_and_brightness_in_json` (`indicators: C`; `--json` with its brightness 0.021) |
| AC-17 | Pass | `lib11_ac17_idle_instruction_weighs_four_on_ti59_and_ti58` (the instructions `profile` counted in 1 s at key readiness × 128 ticks equal the ticks within one instruction), `lib11_ac17_ti58c_instruction_weighs_one` (× 32) |

Coverage of the Calc-U-59 column: AC-14 to AC-17, 4 of 4. Column Done
(AC-15 on Linux and Windows with the CI dispatch).

### Exit check of M-59

Every function of the table of AC-14 through `calcu`:
`lib11_ac14_every_function_of_the_table` and C59-9's tests pass.

### Deviations from the design, chosen by the simpler structure

- **`CALCU59_ROMS_DIR`** names another ROM folder for the harness (as
  `frontends/web/build.sh` reads it): `tests/rom_missing_build.sh` and
  `tests/harness_rom_tests.py` start `calcu` on a folder without one file
  (2.3.0's `ti59.py` read `TI59_ROMS_DIR`).
- **`CalcU59HarnessRomMissing`** (Frequent) takes the place of
  `CalcU59HeadlessRomMissing` (§11.6 H6 named only the removal): LIB-12
  AC-7, AC-10 stay tested.
- **The scripts' output** goes to `tools/build/` (ignored).

### The adapter check (plan §1; §11.5)

Every adapter row of the approved designs about Calc-U-59 or the library,
removed in M-59 or earlier, looked up in the code, and the source search of
§11.5 (outside `docs/`, the feature files and comments that tell history).
`CalcU59NoOldTools` fails if the tools, `ti59headless`, `TIMemoryAdapter`,
`memoryOf(` or `core/TIAdapters.hpp` come back.

| Row (design) | Removed in | Result |
|---|---|---|
| `TIDebugTargetAdapter`, `TIFreezeGate`, `TIFreezeProbe`, `TIKeyTraceObserver`, the interim topic `c59.trace`, `c59.printer-output`, `c59.cut-paper`, LIB-9 A-3 (C59-1, LIB-3, LIB-5, LIB-8, LIB-10, C59-6) | M-53 | Gone (`c59.trace` is now the name of the debug panel's TRACE entry in the control rules, `TIDebugPart`, not a topic) |
| `EmulatorSession::moduleImage()` (C59-1, C59-6 §5) | M-53 | **Finding, fixed in M-59**: still there, read only by two tests; removed with `refreshModule()` |
| `typing::PauseProbe` (LIB-5) | M-53 | **Finding, stays**: the product side (`TIFreezeProbe`) is gone; the library seam `TypingPart::setPauseProbe` has one user, the library's test product (`library/harness/tests/TestProduct.cpp`, its frozen machine). Not an adapter of a product's caller; removing it is a library change of its own, for the user |
| `TIMemoryAdapter` (C59-1, C59-2 §5: "its readers M-59") | M-59 | Gone, with `memoryOf` (the debug log and snapshots on `c59.program-memory`, `registerTop()` added; the core tests and golden runs on `TIMachineTesting`) |
| The TI-58C start of 2.3.0, `TIKeySequencerHook`, `TIKeystrokePlayback`, `SliceRequest::due`, `TIPresetOperation`, `StateFile`, `StateApplier`, the 2.3.0 directives on places, the machine-control commands of `TIAppPart`/`TIWebPart`, `HarnessMachines`, printer connect and module change, `TIFileAccess`'s constant memory, the restart call | M-54 | Gone (`StateApplier`, `StateFile`, `HarnessMachines` are names in comments; `TIFileAccess` stays as the save folder access of the constant memory file, `ConstantMemoryKeeper`, C59-3: no old call behind it; `SliceRequest` is the library pacer's own type, `::due` gone) |
| LIB-7 A-4 to A-7, `TICardReaderAdapter`, `TICardWaitAdapter`, the module list for today's menus, LIB-1's card reader row, `cuecards.txt` | M-55 | Gone (`cuecards` names the module files' cue card sections) |
| `TIDebugPanelAdapter`, the drop flow (`c59.open-dropped`, `c59.drop-action`) | M-56 | Gone (named in comments only) |
| PC-100C buttons, `c59.model`, `c59.web.model`, `--pacing-probe` (59), the settings commands of `TIWebPart`, `c59.web.pointer-*`, A-4 drawing of "ROMs missing" | M-57 | Gone (`--pacing-probe` is an attach client of the Qt6 app; `c59.web.model` is named by a test that checks it is gone) |
| `MachineModel.swift`, the Swift test aids, the Swift copy of the model facts | M-58 | Gone (named by the checks that keep them out) |
| `c59.full-speed` (the display hold's command; C59-8 qa-report) | M-57, M-58 | **Finding, fixed in M-59**: no frontend sent it any more (the hold is the library's `speed`); the command and `TIAppPart::setFullSpeed` removed, the session test sends `speed` |
| `c59.run-state` (`TIRunState`: full speed, keystrokes playing; LIB-10 L-E5) | M-54, M-57 | **Finding, fixed in M-59**: still published; Qt6 (`EmulatorController`) and Swift (`Bridge/TIAppSession.mm`) only mapped its name to a refresh, no field was read (the Web shows full speed from `TIWebPart`'s own topic). The topic and `TIRunState` removed, the two readers' name tests with them (the Qt6 app and the Swift app build) |
| `c59.led` (`TILedState`: the LED frame and the C indicator) | — | **Stays, a product topic**: no adapter row names it (LIB-3 §5's interim list has no `c59.led`); it is the frame sampler's picture of the TI's LEDs that the frontends draw (the library's `display` is the raw output). Read by Qt6 and Swift |
| `c59.cue-card` (`TICueCardState`) | — | **Stays, a product topic**: C59-7 design §5 kept it "as the product topic of C59-2 AC-6 and AC-22, no longer interim" (the module cue card over the fitted one, the corner on show). Read by Qt6 and Swift |
| `c59.printer` (`TIPrinterState`: `connected`, `trace`; LIB-8 P-E1) | M-57 | **Stays, a product topic, for TRACE**: the PC-100C's TRACE position is kept by the app session (C59-6 AC-6), which no library topic carries; Qt6 (`printerTrace`) and Swift read `trace`. Its `connected` field (P-E1) has no reader any more (the bar's printer control is LIB-4's); left in the payload, for the user |
| LIB-8 P-E3, the PC-100C's code lines (`TIMachineTesting::printedCodeLines`) | M-59 (the tool's use) | The tool's use gone with `tools/headless/capi.cpp`; the golden runs keep it, a test inside the core (as the row says) |
| `tools/headless/`, `tools/agentic/` and their CTests | M-59 | Gone (`CalcU59NoOldTools`) |
| `core/TIAdapters.*` (D-17) | M-59 | Renamed `core/TITypingSupport.*`; no file named for adapters |

So no adapter of Calc-U-59 is left in the plan's sense: what stays is
named above with its reason (the library's `PauseProbe` for its test
product, the three product topics, `c59.printer`'s unread `connected`).
With M-51's check this closes plan §1's adapter rule for both products.

### Golden runs

`golden_runs_tests.cpp` passes unchanged (in `CalcU59Tests`); no golden run
differs.
