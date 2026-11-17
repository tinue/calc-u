# QA report: C59-4 Calc-U-59 debug panel

Results per acceptance criterion and edge case of `spec.md`.

## M-56 (2026-11-16, branch `m56-opening-debug-59`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo, in `build/tests` (`-DCALCU_FRONTEND_QT=OFF`) and `build/qt`
(`-DCALCU_FRONTEND_QT=ON`, Homebrew Qt 6, offscreen); the Web `dist/` with
Homebrew Emscripten (`frontends/web/build.sh`, `run-tests.sh`). The Swift
frontend was type-checked only (`xcodebuild` does not run from a worktree;
see "Swift" below).

The code (design §2; all under `products/calc-u-59/` unless named):
- core: `core/TIDebugKinds.hpp` (the product kinds `c59.asm-overlay`
  `cu59::AsmOverlay`, `c59.rom-heatmap` `cu59::RomHeatmapCounter`,
  `c59.calculator-state` `cu59::CalculatorState`, `c59.machine-log`
  `cu59::MachineLog`; `decodeRegister`; found with the library's `calcu::machine::first<K>` since Part E simplification E-7),
  `core/TIDebugKindsImpl.hpp`, `core/TIDebugKinds.cpp`; `TIMachine` offers
  them as `TMC0501/c59.…`, counts the heatmap in `stepOne` while it is on,
  ends the overlay's run at every processor reset and removes the overlay at
  a Cold boot; `TIDebugPanelAdapter` and `debugPanelOf` are gone
  (`core/TIAdapters.*`); `TIMachineTesting::stepToBoundary` for the tests;
- app: `app/TIDebugPart.*` (new: the six product topics, the commands, the
  product entries as LIB-4 `ControlRules`, `freezeButtons`),
  `app/AsmOverlay.*` (the reader of format 2.0, `readAsmOverlay`; 
  `parseAsmWords` and `runAsmOverlay` gone), `app/DebugModel.*` (no heatmap
  and no log any more; the module's image handed in), `app/RomHeatmap.*`
  (the drawing's grid, steps and colours only; `RomHeatmap.cpp` gone),
  `app/FrameSampler.*`, `app/EmulatorSession.hpp` (no debug events, no
  `debugPanel()`), `app/TIAppPart.*`, `TIAppPartFlows.cpp` (the debugger
  flows, the interim topics `c59.debug.*`, `c59.show-debug-panel`, the
  commands `c59.load-asm` … `c59.dump-memory` and the `Change` bits
  `LiveChanged` … `ListingChanged` gone; the part owns `TIDebugPart` and
  hands it to LIB-10's `showPanel` and to LIB-4 as control rules;
  `makeAppSession` puts LIB-4's `ControlPart` into the session),
  `app/TIWebPart.*` (the `c59.web.live`, `.cpu`, `.heatmap`, `.log`, `.asm`
  topics and `asmActive` gone), `app/HarnessProduct.cpp` (the debug part
  among the harness's parts, the commands of AC-22);
- library: `debugger/RunControl::fireStart` (an armed start fired outside
  an advance; see Deviations) with `debugger/tests/start_tests.cpp`
  `c59_4_ac17_fire_start_outside_an_advance`;
- frontends: Qt6 `frontends/qt6/app/EmulatorController.*` (the product
  topics, the LOG tab's `RingLog` view of LIB-3's `log`, the product
  entries), `debug/DebugPanel.cpp`, `LiveDebugWidget.cpp`,
  `CpuInspectorWidget.cpp`, `AsmOverlayBox.cpp`, `LogWidget.cpp`,
  `frontends/qt6/CMakeLists.txt`; Web `frontends/web/bindings.cpp`
  (LIB-4's `ControlPart` in the session), `src/engine.js`,
  `src/Debugger.jsx`; Swift `frontends/swift/Bridge/TIAppSession.*`,
  `App/EmulatorViewModel.swift`, `App/Views/DebugView.swift`,
  `LiveDebugView.swift`, `CPUInspectorView.swift`;
- examples: the five overlays in format 2.0 (`examples/assembly/*.asm`,
  `examples/screentest/test_stopwatch.asm`), `examples/assembly/README.md`;
  the instructions 2.3.0's reader gave them, recorded before the reader
  changed: `tests/fixtures/asm/*.words`;
- tests: `tests/debug_panel_tests.cpp` (49), `tests/asm_overlay_tests.cpp`
  (16), five cases of `tests/harness_tests.cpp`; the Web cases of
  `frontends/web/tests/play-calculator.spec.js`; Swift
  `Calc-U-59UITests/DebugPanelTests.swift`, `ScreenshotTests.swift`.

Result of the run: `build/tests` Frequent all CTest entries passed
(`CalcU59Tests` 480 `TEST`s, 0 failed, 21 s in a `ctest -j8`;
`calcu_library_tests` 944); `build/qt` Frequent passed (offscreen);
`CalcU59WebPlaywright` 103 passed, 0 failed, 5 skipped (the conditional
pacing, prerender and rom-missing ones). The golden replays (in
`CalcU59Tests`) are equal; the reference runs: see "Golden and reference
runs". CI was not dispatched (plan §1).

| ID | Result | Test case (`tests/debug_panel_tests.cpp` unless named) |
|---|---|---|
| AC-1 | Pass | `c59_4_ac1_panel_fed_by_product_and_generic_topics`; the three frontends draw the product topics and LIB-3's `log`, LIB-10's `run-state` and `trace` (Web: `C59-4 AC-21 …`) |
| AC-2 | Pass | `c59_4_ac2_view_shown`, `_calculator_passed_over_while_overlay_runs`; `debugger_tests.cpp` `lib10_ac32_debug_panel_calculator_cpu_log`, `session_tests.cpp` `lib3_ac30_show_debug_panel_is_product_message` |
| AC-3 | Pass | `c59_4_ac3_shows_registers_partition_steps_flags_source_scom_returns` (2.3.0's `live_snapshot_and_listing`), `_ti58c_shows_h00_to_h03`, `_read_through_the_target` |
| AC-4 | Pass | `c59_4_ac4_listing_while_paused_memory_rom_module`, `_current_step_marked` |
| AC-5 | Pass | `c59_4_ac5_freeze_on_start_arms_then_armed_disarms`, `_freeze_and_start_only_while_running_and_not_armed`; Web `C59-4 AC-5 FREEZE, STEP and RESUME on the CALCULATOR tab` |
| AC-6 | Pass | `c59_4_ac6_newest_32_running_up_to_1024_and_next_paused` |
| AC-7 | Pass | `c59_4_ac7_cpu_tab_buttons_for_cpu` |
| AC-8 | Pass | `c59_4_ac8_counts_each_rom_address_once` (2.3.0's `rom_heatmap_counts_new_frames_once`: now the core counts each instruction once), `_reset_by_command_reset_and_model_switch`, `_apart_from_the_profile` |
| AC-9 | Pass | `c59_4_ac9_last_2000_lines_in_pages` (2.3.0's `ring_log_display_pages`), `_clear_empties_the_view_not_the_log` |
| AC-10 | Pass | `c59_4_ac10_dumps_in_todays_layout` (2.3.0's `debug_dumps`, and the command's reply and log lines) |
| AC-11 | Pass | `c59_4_ac11_levels_off_info_debug`, `_debug_logs_each_register_and_memory_write`, `_product_lines_at_every_level` |
| AC-12 | Pass | `c59_4_ac12_trace_button_starts_and_ends_the_button_trace`, `_shows_a_trace_whoever_started_it`, `_cannot_be_used_without_a_writable_folder`; Web: the traces handed over are listed (unchanged, `LIB-10 AC-26` cases) |
| AC-13 | Pass | `c59_4_ac13_up_to_2048_over_1800_to_1fff`, `_taken_away`, `_entered_at_1800` |
| AC-14 | Pass | `asm_overlay_tests.cpp`: `c59_4_ac14_first_line_version`, `_prose_before_hex_not_read`, `_semicolon_comments`, `_runs_of_four_eight_twelve`, `_0x_prefix_either_case`, `_crlf`, `_first_at_1800` |
| AC-15 | Pass | `asm_overlay_tests.cpp`: `c59_4_ac15_outdated_names_the_format_version_and_where`, `_no_hex_line`, `_nothing_after_hex`, `_run_not_a_multiple_of_four`, `_above_1fff`, `_neither_hex_nor_comment`, `_more_than_2048` |
| AC-16 | Pass | `c59_4_ac16_select_file_loads_at_once`, `_run_only_with_instructions`, `_clear_resets_when_running`; Web `C59-4 AC-16 an overlay is chosen and run and the CPU tab shows it` |
| AC-17 | Pass (see Deviations) | `c59_4_ac17_enters_at_1800_with_cond` (`DPT.asm`), `_armed_cpu_start_pauses_at_1800`, `_while_paused_not_while_operation`, `_fails_after_8192_with_status_and_notice` (the core's entry not reached; the part's sentence) |
| AC-18 | Pass | `c59_4_ac18_calculator_tab_closed_cpu_shown`, `_freeze_step_resume_inside_the_overlay_it_still_runs`, `_reset_ends_run_keeps_instructions_run_again` |
| AC-19 | Pass | `c59_4_ac19_asm_opened_as_by_select_file` (`open` by drop, start and harness); `opening_tests.cpp` `lib13_ac13_asm_is_c59_4s_kind`; `harness_tests.cpp` `lib13_ac9_open_takes_every_calc_u_59_file` |
| AC-20 | Pass for the five shipped overlays; the hosted copies wait for M-65 | `asm_overlay_tests.cpp`: `c59_4_ac20_five_overlays_in_2_0_give_2_3_0s_instructions`, `_dpt_runs_with_cond`; `examples/assembly/README.md` describes 2.0 |
| AC-21 | Pass (Qt6, Web, Swift) | `c59_4_ac21_labels_and_short_forms`; Web `C59-4 AC-21 the three tabs, labels and short forms`; Swift `DebugPanelTests.testC59_4_AC21TabsAndLabels` (passes on the iPhone 18 Pro Max simulator since M-56's fix, `UIRegressionTests` of M-57 and M-58); Qt6: the widgets keep their labels and short forms (`DEBUG`, `F.START`), unchanged code, `CalcU59QtTests` builds them; the attach scenario comes with M-57 |
| AC-22 | Pass | `harness_tests.cpp`: `c59_4_ac22_asm_load_run_clear`, `_dump_replies_lines`, `_log_level`, `_heatmap_reset`, `_no_preset_steps_for_them` |
| AC-23 | Pass | the cases of 2.3.0's `shared_debug_tests.cpp:184-287` named in AC-3, AC-8, AC-9, AC-10, AC-14; `asm_overlay_parser` without its two lenient cases (now refused, AC-15 rows); `asm_overlay_runs` as `c59_4_ac13_entered_at_1800` |
| AC-24 | Pass (Web; Qt6 since M-57; Swift: `testC59_4_AC5…` passes on the iPhone simulator, R/S through attach since M-58; `testC59_4_AC16AsmOverlay` is in the Screenshots plan, not run) | Qt6 `CalcU59QtTests` `c59_4_ac5_qt6_freeze_step_resume_on_the_calculator_tab`, `c59_4_ac16_qt6_overlay_chosen_run_cpu_tab_shows_it` (Select File through the shell's file chooser, Run, "ASM entered at 0x1800", the CPU history at 18xx; M-57); Web `C59-4 AC-5 …`, `C59-4 AC-16 …`; Swift `DebugPanelTests.testC59_4_AC5CalculatorFreezeStepResume`, `ScreenshotTests.testC59_4_AC16AsmOverlay` (Screenshots plan) |
| AC-25 | Pass | `c59_4_ac25_six_product_topics_read_by_state`, `_entries_asm_run_trace_calculator_tab_with_reasons`, `_cpu_and_calculator_computed_while_shown`; `harness_tests.cpp` `c59_4_ac22_heatmap_reset` (`state calculator-debug`, `state cpu-debug`) |
| AC-26 | Pass | `c59_4_ac26_cold_boot_removes`, `_new_machine_by_preset_and_model_switch_removes` (replaces `part_flows_tests.cpp` `part_asm_overlay`) |
| EC-1 | Pass | `c59_4_ec1_run_without_instructions_status` |
| EC-2 | Pass | `c59_4_ec2_refused_file_keeps_loaded` |
| EC-3 | Pass | `c59_4_ec3_step_of_the_other_level_cannot_be_used_preset_step_is_carried_out` |
| EC-4 | Pass | `c59_4_ec4_view_kept_until_shown` (a late listener gets `debug-view`); Web: the portrait layout keeps the tab |
| EC-5 | Pass | `c59_4_ec5_opened_before_first_machine_kept` |

Coverage: every AC and EC has its test. Column status: In progress. The
plan makes C59-4 Done in M-65 (AC-20's hosted copies with the site); AC-24's
Qt6 scenarios pass since M-57 (on the real window); the Swift UI and unit
tests run from the main checkout and pass (M-56's fix below; M-58: R/S
through attach, the hidden `btn-key-rs` gone with C59-8 AC-21).

### Swift

Type-checked only: the bridge (`clang++ -fsyntax-only -std=gnu++20 -x
objective-c++ -fobjc-arc`, macOS Debug and Release, iOS simulator), the app,
the unit tests and the UI tests (`swiftc -typecheck` with the bridging
headers, macOS and iOS simulator); `check_xcode_project.sh`,
`check_no_own_session.sh`, `check_no_attach.sh --sources` print ok. Not
built or linked, no test run. To run from the main checkout
(`products/calc-u-59/frontends/swift`): the macOS and iOS simulator builds,
`-testPlan UnitTests` and `-testPlan UIRegressionTests`; `bin/simulators
setup` first, so the simulator's `test_stopwatch.asm` is the 2.0 file.

Run from the main checkout (2026-11-17): `DebugPanelTests.testC59_4_AC5…`
failed at "RESUME after STEP is not enabled". Not the run control (the
harness pauses and steps the same run with owner `program`), but the
CALCULATOR tab's frozen PROGRAM STEPS list: an eager `VStack` of the whole
program memory (960 rows, five texts each), so while paused every XCUITest
query took about 3 s and the 5 s waits ran out. The list is a `LazyVStack`
now (queries about 0.05 s, the test 18 s instead of 47 s); the scenario
presses R/S (`btn-key-rs`) before FREEZE, so the empty program memory runs
and the pause lands on a program step, as the Web scenario runs A.

### Golden and reference runs

The golden replays do not use the overlay or the panel and are equal
(`golden_runs_tests.cpp`). The heatmap counts only while it is on, so the
fast path of `TIMachine::advance` gains one test of a flag per instruction.
The reference runs (`CalcU59ReferenceRuns`): 0 of 57 examples differ, the
8 findings named since M-54 as before, 4 card examples wait for M-55; the
debug examples (`debug_*`, `screentest_test_cdebug`) are the same.

### Deviations from the design, chosen by the simpler structure

- **One file pair for the four core kinds** (`core/TIDebugKinds.hpp`,
  `TIDebugKindsImpl.hpp`, `TIDebugKinds.cpp`) instead of four pairs; the
  capability classes are `cu59::AsmOverlay`, `RomHeatmapCounter`,
  `CalculatorState`, `MachineLog` (the file kind of LIB-13 keeps the name
  `asm-overlay`, its class is internal to `app/TIFileKinds.cpp`).
- **The CPU tab reads the frame ring through `c59.calculator-state`**, not
  the debug target's `history`: its frames hold `Sout`, the post-execution
  digit and the display state, which the target names no register for
  (design §3.4 said history; D-3's rule applied to them).
- **AC-17's pause at 1800: `RunControl::fireStart(level)`** (a small LIB-10
  addition) instead of D-8's entry mark through `TILevelRules`: the entry
  runs outside an advance, where no observer is asked; `fireStart` pauses
  with the reason pause-on-start only when that level is armed.
  `TILevelRules::landNextPauseAtOnce` is no longer used by the overlay.
- **No `debug-text` request (D-7):** each product topic's value is the
  Web page's form (the formatted rows, the heatmap's steps as bytes); the
  native frontends read the payload's fields. The Web keeps
  `c59.web.debug-font` (the panel's font size by its width; not panel data).
- **The heatmap is reset when the CPU tab comes into view** by
  `c59.cpu-debug {on}` itself (as today), not by a second command from the
  frontend; `c59.heatmap-reset` is the CLR button and the harness's.
- **The machine's log lines of one host call are one log message**, the
  lines parted by `\n` (at the level debug the TI writes thousands per
  second); the frontends split them into the LOG tab's lines.
- **`debug-view` is a state topic with a count of requests**: several
  `debug-panel:` steps in one call arrive as the last one (the stream sent
  each); the frontends switch when the count grows.
- **Status texts**: "Loaded n word(s) at 0x1800." kept as today; an overlay
  removed by a Cold boot says "ASM overlay removed by the Cold boot.", by a
  new machine "ASM overlay removed with the machine.".
- **AC-17's failure path** is tested at the core (`enter` with no
  instruction allowed does not enter) and by the part's sentence; a real
  overlay whose entry does not come within 8192 instructions was not found
  for a session test.

### Findings

- `tools/agentic/asm59.py` assembles keystroke programs (keycodes), not
  TMC0501 overlays: it writes no ASM overlay file, so nothing changed there
  (design §7 assumed it writes overlays).
