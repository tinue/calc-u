# LIB-10: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-40, 2026-11-15, branch `m40-debugger`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo: `build/tests` (`-DCALCU_FRONTEND_QT=OFF`) and `build/qt`
(Qt6 from Homebrew), and the library's Web test product in `build/web`
(Homebrew Emscripten; Playwright in Chromium and WebKit).

The code:
- `library/machine/DebugTarget.hpp` (new): the debug-target interface and
  the machine's side (`InstructionEvent`, `InstructionObserver`, `Checks`,
  `DebugStop`, `Debugging`); `Machine::debugging()` (default `nullptr`)
  and `Machine::debugTarget(id)`; `checkDebugging()` in
  `machine/Conformance.*`.
- `library/debugger/` (new area, `calcu::debugger`, in `calcu_library`):
  `Levels.*` (`LevelRules`, `CpuOnlyRules`), `Breakpoints.*`
  (`BreakpointTable`), `RunControl.*` (the gate and G1 to G14),
  `RunState.*` (the `run-state` and `trace` payloads and the session's
  names), `TraceSession.*` (`TraceFile`, `TraceRecorder`,
  `TraceSession`), `Profile.*`, `Symbols.*`, `DebuggerPart.*` (the
  session commands and requests of §3.6), `DebuggerSteps.cpp` (the steps
  of AC-19 on LIB-6's step place), `ButtonRule.*`.
- `library/harness/DebugCommands.cpp` (new), `Commands.hpp`,
  `FirstCommands.cpp` (`wait`, AC-21), `Product.hpp`
  (`HarnessProduct::debuggerPart()`), `Run.cpp` (the run's trace folder
  and limit, the steps in the vocabulary).
- `library/qt6/RunControlButtons.*` (in `calcu_qt6`) and
  `library/web/components/RunControlButtons.jsx` (in
  `COMPONENT_SOURCES`).
- Sessions (each its own commit): `SessionPart::machineRestarted` with
  `sessions::Restart` and `SessionContext::tellRestarted`
  (`SessionPart.hpp`, `AppSession.cpp`; LIB-18 §8); an operation's
  `OperationContext::run` gives way while the run control pauses
  (`Operation.cpp`); `MachineSession::setMachine` clears the paused flag
  (`MachineSession.*`). `startup/StartupPart.cpp` tells the parts of a
  Reset and a Cold boot.
- Test stand-ins: the test processor `library/machine/tests/TestCpu.*`
  behind `test-seg`'s `test-cpu` and `test-dot`'s `main` and `inner`,
  switched on by the build option `TestProgram` (or `enableCpu`); the
  test rules, trace format and symbol table
  (`library/debugger/tests/TestDebugParts.hpp`), the session rig
  (`DebugRig.hpp`: the preset rig's parts with the DebuggerPart), the
  rule's table `button_cases.json`; the harness's test product got the
  test processor in both models and a DebuggerPart; the library's Web test
  product a DebuggerPart without a trace folder (hand-over).

The tests: `library/machine/tests/debug_target_tests.cpp`,
`library/debugger/tests/*_tests.cpp`,
`library/harness/tests/debug_commands_tests.cpp` in `calcu_library_tests`
(Frequent; 98 new `TEST`s, 727 in all), the CTest entry
`calcu_cli.ac50_refused_expect_ends_one_shot_nonzero` (Frequent),
`library/qt6/tests/run_control_buttons_tests.cpp` in `calcu_qt6_tests`
(Frequent, offscreen) and `library/web/tests/run-control.spec.js` in
`LibraryWebPlaywright` (Longer, Chromium and WebKit).

| ID | Result | Evidence |
|---|---|---|
| AC-1 | Pass | `ac1_one_target_per_processor_named_as_descriptor` (one target on test-seg, two on test-dot, named as the descriptor lists them; none without the option), `ac1_target_states_registers_spaces_code_space_offers_qualifier_now`, `conformance_check_debugging` (`machine/tests/debug_target_tests.cpp`); `ac20_targets_registers_set_register` (the `targets` block) |
| AC-2 | Pass | `ac2_registers_read_written_where_writable`, `ac2_peek_no_effect_unreadable_device_cell`, `ac2_refused_poke_changes_nothing`; `ac20_peek_poke_marks_unreadable_refuses_at_first` |
| AC-3 | Pass | `ac3_next_address_and_pre_execution_state` |
| AC-4 | Pass | `ac4_history_newest_first_with_registers_before` (newest first, the registers before, 8 kept); `ac20_history_disassemble_with_text` |
| AC-5 | Pass | `ac5_session_has_one_run_control_unused`, `ac5_paused_no_emulated_time_display_kept`, `ac5_stopped_advance_gives_ticks_run` (`debugger/tests/run_control_tests.cpp`); a preset's `wait:` on the paused machine passes in host time and ends paused: `ac13_armed_start_fires_in_operation` |
| AC-6 | Pass | `ac6_pause_lands_where_rules_say_reply_then` (the reply when `program` has landed at the next boundary; `cpu` at once), `ac6_resume_whichever_level_owned`, `ac6_unknown_level_refused` |
| AC-7 | Pass | `ac7_cpu_step_one_instruction_of_named_or_current_target`, `ac7_cpu_step_repeats_waiting_instruction_by_rule`, `ac7_program_step_by_rule`, `ac7_running_while_stepping`, `ac7_step_refused_when_not_paused`, `ac7_hit_ends_steps` |
| AC-8 | Pass | `ac8_step_over_call_runs_routine`, `ac8_step_out_runs_to_return`, `ac8_breakpoint_inside_still_pauses`, `ac8_refused_without_offer` (port of Calc-U-1600 `run_control_tests.cpp:159` onto the test processor); `ac20_step_over_out_refused_without_offer` |
| AC-9 | Pass | `ac9_armed_fires_at_rule_event_once`, `ac9_arming_one_disarms_other_off_both_pause_both`, `ac9_default_cpu_rule_leaves_key_readiness` (`debugger/tests/start_tests.cpp`) |
| AC-10 | Pass | `ac10_pauses_before_instruction`, `ac10_qualifier_narrows`, `ac10_resume_and_step_pass_it_once`, `ac10_two_targets_only_the_one_to_run_passes` (`debugger/tests/breakpoint_tests.cpp`; ports of `debug_target_tests.cpp:111-193, 464`) |
| AC-11 | Pass | `ac11_read_write_both_pause_after_instruction`, `ac11_fetch_is_no_access`, `ac11_refused_without_offer` (ports of `debug_target_tests.cpp:269-301`) |
| AC-12 | Pass | `ac12_test_says_go_on` (a setter's `HitTest` asked at each hit; the machine went on twice) |
| AC-13 | Pass | `ac13_no_breakpoint_or_watch_in_operation`, `ac13_commanded_pause_step_resume_in_operation` (a preset's `pause:`, `step:`, `resume:`), `ac13_armed_start_fires_in_operation` (the library form of `test_cdebug.ti59`: armed inside the load, fired by the key that starts the program, inside its hold, C-1 of FS 5) |
| AC-14 | Pass | `ac14_pause_stops_all_targets_names_current` (test-dot: neither target runs; the run state names the current one) |
| AC-15 | Pass | `ac15_reset_and_cold_boot_while_paused_run`, `ac15_armed_stays_across_reset`, `ac15_new_machine_not_paused_nothing_armed`, `ac15_breakpoints_rebound_by_target_name_rest_dropped` |
| AC-16 | Pass | `ac16_resume_sets_clocks_and_starts_afresh` (paced: test-dot's clock set from the host once, pacing started afresh, the pause not made up), `ac16_paused_survives_suspend_and_continue` |
| AC-17 | Pass | `ac17_run_state_fields_for_each_reason` (pause, step, breakpoint and watch with the IDs and setters, pause-on-start, entry; the armed level, pause-at-start) |
| AC-18 | Pass | `ac18_rule_cases_from_table`, `ac18_step_repeats_four_a_second_while_held` (`debugger/tests/button_rule_tests.cpp`); `ac18_qt6_buttons_follow_run_state_and_send_commands` (`calcu_qt6_tests`, offscreen: the table, an app session, a held step repeating at 250 and 500 ms, the product's labels); `ac18 react buttons follow the run state` (`LibraryWebPlaywright`, Chromium and WebKit: the table, the test product's session, a held step). Placed in a shell by LIB-14 (M-43) and by the products (M-45, M-53, M-58) |
| AC-19 | Pass | `ac19_each_step_does_what_its_ac_says`, `ac19_level_view_count_out_of_range_refused_at_read`, `ac19_step_not_paused_logs_and_goes_on` (OQ-4), `ac19_undefined_steps_refused_for_a_model` (test-dot stands for a Sharp model), `ac19_trace_defined_for_every_model`, `ac19_no_step_waits_for_a_running_machine`, `ac19_debug_panel_is_a_frontend_step` (`debugger/tests/steps_tests.cpp`). With them LIB-6 AC-19's debugger steps and AC-63's `debug-panel:` pass (LIB-6 qa-report updated) |
| AC-20 | Pass | `ac20_targets_registers_set_register`, `ac20_peek_poke_marks_unreadable_refuses_at_first`, `ac20_break_watch_on_off_list_own_only`, `ac20_history_disassemble_with_text`, `ac20_profile_counts_and_report`, `ac20_step_over_out_refused_without_offer`, `ac20_pause_at_start`, `ac20_state_run_state_and_trace`, `ac20_step_not_paused_refused` (`harness/tests/debug_commands_tests.cpp`, the test product) |
| AC-21 | Pass | `ac21_wait_ends_at_pause_gives_time_and_state`, `ac21_wait_while_paused_ends_at_once` |
| AC-22 | Pass | `ac22_trace_records_every_instruction_until_off`, `ac22_ends_with_its_preset_or_new_machine`, `ac22_replaces_file_one_at_a_time`, `ac22_slash_refused_at_read`, `ac22_cannot_make_file_step_fails` (`debugger/tests/trace_tests.cpp`) |
| AC-23 | Pass | `ac23_button_name_is_products` (`trace {button: true}`: the product's name, replaced at each start). The products' buttons send it in M-45 and M-53 |
| AC-24 | Pass | `ac24_limit_ends_complete_file_notice` (1 MB: the file ends complete within the limit, the notice of D-11), `ac24_new_machine_log_line` |
| AC-25 | Pass | `ac25_trace_topic_whoever_started` (the command, the button, a preset's, paced) |
| AC-26 | Pass (library part) | `ac26_ended_trace_in_memory_handed_with_keep_file` (no folder, hand-over: `KeepFile` with the name, the bytes and the media type; without hand-over the start fails, D-10); `ac26 an ended trace is a download` (`LibraryWebPlaywright`, LIB-15's test product, Chromium and WebKit). Calc-U-59's Web uses it in M-53 |
| AC-45 | Pass | `ac45_other_level_step_takes_the_pause` |
| AC-46 | Pass | `ac46_paused_before_first_instruction_after_reset_cold_boot_build`, `ac46_once_and_separate_from_pause_on_start` |
| AC-47 | Pass | `ac47_ids_owners_remove_only_own`, `ac47_hit_names_ids_and_owners`; `ac20_break_watch_on_off_list_own_only` (the harness's `break off` leaves a debugger's) |
| AC-48 | Pass | `ac48_written_view_last_value_or_none_no_effect`; `ac20_peek_poke_marks_unreadable_refuses_at_first` (`io-written` cell with none) |
| AC-49 | Pass | `ac49_every_bank_and_vbank_by_name_no_switch`, `ac49_range_across_banks_refused`; through the commands `ac20_peek_poke_marks_unreadable_refuses_at_first` (`bank=`, `vbank=`, a range across banks refused, no bank switched) |
| AC-50 | Pass | `ac50_peek_into_file_gives_count`, `ac50_expect_register_refused_with_value`, `ac50_expect_memory_refused_at_first_difference`; `calcu_cli.ac50_refused_expect_ends_one_shot_nonzero` (exit status 1, the commands after it not run) |
| AC-51 | Pass | `ac51_symbol_names_as_addresses_and_in_output` (`break`, `disassemble`, `peek`, `break off` by name; `name+offset` in `history` and `disassemble`), `ac51_unknown_name_refused` (an unknown name; a file no reader takes) |
| EC-1 | Pass | `ec1_pause_while_paused_done_nothing_changes` |
| EC-2 | Pass | `ec2_resume_while_running_done` |
| EC-3 | Pass | `ec3_past_end_of_space_refused`; `ac20_break_watch_on_off_list_own_only` |
| EC-4 | Pass | `ec4_several_hits_pause_once_lowest_address_named` (a two-cell store hits two watches: one pause, the lower address named) |
| EC-5 | Pass | `ec5_step_from_breakpoint_carries_it_out` |
| EC-6 | Pass | `ec6_large_count_ended_by_pause_reset_new_machine` |
| EC-7 | Pass | `ec7_preset_ends_paused_stays_paused` |
| EC-8 | Pass | `ec8_no_writable_folder_step_fails_with_reason` (no folder; a folder that cannot be written) |
| EC-9 | Pass | `ec9_peek_poke_outside_space_refused`; `ac20_peek_poke_marks_unreadable_refuses_at_first` |

Column status: every criterion (AC-1 to AC-26, AC-45 to AC-51) and edge
case (EC-1 to EC-9) of the Library column passes. The Library column is
**Done** (plan §5.1: M-40). The product columns are worked in M-45
(Calc-U-1600) and M-53 (Calc-U-59).

### Exit check of M-40

Library tests: `calcu_library_tests` 727 `TEST`s, 0 failed; Frequent in
`build/tests` (31 entries) and `build/qt` (32 entries, with
`calcu_qt6_tests` at 11 `TEST`s) pass; `LibraryWebPlaywright` 96 passed,
2 skipped (the run-control cases in both engines among them);
`LibraryWebNodeTests` pass. No product holds a DebuggerPart yet (design
§4): their harness entries are unchanged.

### Golden runs

M-40 touches no core and no product code. Both products' golden replays
pass unchanged in their Frequent suites (`CalcU59Tests`, `CoreTests`).
`Machine::debugging()` has a default, so both products' machines build
unchanged.

### Deviations from the design, chosen by the simpler structure

- **Where the restart is told.** Reset and Cold boot are carried out in
  `StartupPart::restart` (LIB-18), not in `MachineControl`; the parts are
  told from there, through `SessionContext::tellRestarted` (new, in
  `sessions`), which the app session implements by calling
  `SessionPart::machineRestarted` on every part. A power cycle is no
  restart (AC-46 names Reset, Cold boot and a build).
- **An operation on a paused machine (§3.11, G12, D-8).** Two more
  `sessions` changes than §3.11 lists, each in its own commit:
  `OperationContext::run` gives the host its call and ends with what ran
  while the run control keeps the machine paused (before, it failed with
  "No machine is running."), so that a typing wait looks at its key again
  (the key of C-1 comes up on the paused machine) and what needs the
  machine waits until it runs; and `MachineSession::setMachine` clears the
  paused flag (AC-15), so that an operation that builds a new machine runs
  it at once. In virtual time the operation then does not finish, as D-8
  says; its reply carries no run state (D-8 says it does): the run state
  is read with `state run-state`.
- **Interface details.** `Debugging` has a `const target(i)` overload
  (the conformance check reads through it). `LevelRules` and
  `TraceRecorder` have public virtual destructors (owned by the part and
  by a `unique_ptr`). `TraceRecorder::machineGone()` is new: on a new
  machine the trace ends without touching the old machine's core.
  `DebuggerSetup` holds `TraceSetup` (recorder, folder as
  `Paths::ensure` gives it, limit, hand-over), the symbol readers, and
  per model whether the run-control steps are defined, the panel's views
  and the frontend's `showPanel`; `setTraceFolder` and `setTraceLimit`
  for a host that makes its folder later (the harness).
- **The default `cpu` start rule after a restart.** A restart arms the
  rule afresh (`startArmed`), so a Reset by itself does not count as the
  processor leaving key readiness; the TI rules of AC-30 override it in
  M-53.
- **Entry does not disarm.** The pause at the next start (AC-46) leaves
  an armed pause-on-start armed: the two stay separate (AC-46); every
  other pause disarms both (AC-9).
- **Run-control commands need a target.** `pause`, `step`, `resume`,
  `pause-on-start`, `profile` and the target commands are refused with
  "The machine offers no debug target." on a machine whose
  `debugging()` is `nullptr`, as §4 says for the products until their
  columns.
- **Harness forms.** `step`, `step <count>`, `step <level> [<count>]` and
  `step { level: …, count: … }`; `pause` without a level is `cpu`. `wait`
  gives the run state when the machine is paused at its end (it ended at
  a pause, or at once on a paused machine); a wait that ran its full time
  keeps LIB-11's reply. `peek … file=` writes a relative path into the
  run's folder for traces and screenshots (LIB-11 AC-8). `break off
  <target> <address>` takes the harness's breakpoints at that address. A
  `poke` refused at a cell writes back the cells before it, so a refused
  poke changes nothing. A watch on a `-written` view is refused (the view
  is read-only).
- **The test processor.** Beyond §6: `st2` (two cells in one
  instruction, for EC-4), the option `bare` (targets without the optional
  parts, for the refusals of AC-8, AC-11 and AC-20), the inner target's
  program at 0x50 (a loop on itself unless given). The machine's tick
  count and key readiness are kept per instruction while the processor
  runs, so the observer sees the machine after each instruction. The
  harness's test product runs it in both models.
- **The Playwright projects.** `run-control-chromium` and
  `run-control-webkit` in `library/web/tests/playwright.config.js`; the
  rule is reached in the page as `CalcUTest.buttonUse` (the one global of
  the test product's `calc.js`).

## Calc-U-1600 (M-45, 2026-11-16, branch `m45-paper-debugger-1600`)

Built and run as LIB-8's Calc-U-1600 column (same trees; Frequent 33 of
33 in `build/tests`, 35 of 35 in `build/qt`; Longer: `CalcU1600Shots.lib10-ac41`,
`CalcU1600DapSmoke`, `CalcU1600DebuggerSpeed`, the Web Playwright cases).
The column is **In progress**: it is Done in M-46, where the format 2
clauses of AC-40 and AC-43 are tested (design §10.1).

The code (design §10.3):
- `core/Debug/SharpDebugging.*` (new): `SharpDebugging` (one per model,
  owned by the interface machine after its core) and `SharpDebugTarget`
  (one per CPU, over the `CpuView`s); `DebugTarget.*`,
  `MachineDebugTargets.*` deleted; their shared types are in
  `CpuRegisters.hpp`; `SharpMachine::advance` runs `SharpDebugging::run`
  while a check is on, the core's own path otherwise;
- the `-written` views: `LH5801::me1Written()` (every `storeME1`),
  `SC7852::ioWritten()` (every OUT, through `ioWrite`);
- traces: `PC1500TraceFile` keeps whole records in a buffer (buffer mode)
  besides its file mode (today's tools); the product kind
  `c1600.trace-records` (`TraceRecords`) replaces `c1600.trace`;
  `app/SharpTraceRecorder.*`; a version 1 preset's `trace:` goes to
  `PresetFile::traceHook` (L-A3) and from there to the trace session;
- `app/SharpDebuggerSetup.*`, `app/SharpSymbolReaders.*` (new);
  `app/SharpMachineParts.*` deleted; `SharpAppPart` holds the
  `DebuggerPart` (the machine session's gate), tells restarts (L-A2);
  `HarnessProduct` holds one; `AppPaths::traceDir()` is empty on the Web
  (LIB-9 A-3 gone);
- `library/debugger/RunControl`: `step` takes a `StepTest` (D-16);
- `app/dap/` (new, C1600-3's folder, D-15): `DapTargets`, `DapBreakpoints`
  (from `core/Debug/BreakpointTable`), `DapStepping` (from
  `core/Debug/RunControl`);
- Qt6: `DebugController` on `app/dap/` and the library's run control
  (`DapRun`; L-A1), stops from the `run-state` topic after each host call;
  `DapSession` on the new types; `MachineController` is no
  `SharpDebugger`; `MainWindow`: the title from `run-state`, Machine ▸
  Resume (L-A4); `DebugPanel`: TRACE sends `trace {button}`/`{off}`;
- Web: Machine ▸ Resume from `run-state` (L-A4); the engine's `pause` and
  `resume`.

| AC | Result | Test |
|---|---|---|
| AC-36 | Pass | `debug_target_tests.cpp`: `lib10_ac36_pc1500_and_pc1500a_have_the_lh5801`, `lib10_ac36_pc1600_has_the_sc7852_and_the_lh5803`, `lib10_ac36_registers_spaces_offers_history_32`, `lib10_ac36_qualifier_fields_bank_me_pu_pv`, `lib10_ac36_conformance_on_every_model` (`checkDebugging` on PC-1500 A01/A03/A04, PC-1500A, PC-1600 New/Old) |
| AC-37 | Pass | `lib10_ac37_rom_empty_cells_and_io_refuse_a_write`, `lib10_ac37_me1_never_written_device_cells_unreadable`, `lib10_ac37_lh5803_written_only_in_ram`, `lib10_ac37_refused_poke_changes_nothing` |
| AC-38 | Pass | `debugger_tests.cpp`: `lib10_ac38_cpu_only_pause_lands_before_the_next_instruction`, `lib10_ac38_program_level_refused`, `lib10_ac38_pause_on_start_fires_leaving_key_readiness_pc1500`, `_pc1500a`, `_pc1600` (D-17: `CpuOnlyRules`) |
| AC-39 | Pass | `lib10_ac39_step_of_one_target_other_runs_with_the_bus`, `lib10_ac39_only_the_target_about_to_run_passes_its_breakpoint`; `lib10_checks_off_take_the_old_path` (AC-5: equal ticks, memory and retired count with and without the gate) |
| AC-40 | Pass (M-45 parts); the format 2 clauses in M-46 | `harness_tests.cpp`: `lib10_ac40_harness_pause_step_resume`, `lib10_ac40_harness_breakpoint_and_watch`; `debugger_tests.cpp`: `lib10_ac40_run_control_without_a_debugger`; `session_tests.cpp` `lib3_ac38_mirror_holds_every_callback` (pause stops time, resume goes on); Web `play.spec.js` `lib10_ac40 run control pauses and resumes the web machine`. Waits for M-46: `lib10_ac40_debugger_steps_refused_at_read`, `lib10_ac40_trace_step_fails_on_the_web` (presets of format 2) |
| AC-41 | Pass | Qt6 `CalcU1600Shots.lib10-ac41` (`command: pause`, `expect-title: Calc-U-1600 — Paused (debugger)`, `action: Machine > Resume`, `expect-title: Calc-U-1600`); Web `play.spec.js` `lib10_ac41 machine resume` |
| AC-42 | Pass | `debugger_tests.cpp`: `lib10_ac42_trace_step_produces_wellformed_file`, `_left_open_is_closed_at_the_end`, `lib10_ac42_z80_frame_shares_one_file_with_lh5801_frame`, `lib10_ac42_pc1600_trace_step_produces_wellformed_file`, `lib10_ac42_pc1600_left_open_is_closed` (version 1 presets through the trace session), `lib10_ac42_bytes_equal_the_recorded_reference`, `lib10_ac23_button_trace_is_trace_bin`; CTest `CalcU1600ReadTrace` (Frequent, 0.3 s); `presetloader_trace_tests.cpp` keeps the core's file mode for today's tools (`lib10_ac42_tools_…`) |
| AC-43 | Pass (version 1); again on format 2 in M-46 | `lib10_ac43_preset_trace_ends_at_the_limit_with_a_notice`, `lib10_ac43_button_trace_ends_at_the_limit_with_a_notice` (1 MiB: the file ends between 1'000'000 bytes and 1 MiB, well formed, with the notice) |
| AC-44 | Pass | `debug_target_tests.cpp:` today's cases renamed `lib10_ac44_…` on `machine::Debugging` (`DebugTargetTestSupport.hpp`'s `Target` drives `setChecks`, `run`, `takeStop`, `passOnce` and the observer); `run_control_tests.cpp`: `lib10_ac44_step_over_a_call`, `lib10_ac44_data_breakpoint_and_pause` on the library's run control through `MemtestRig.hpp` |
| AC-52 | Pass | `lib10_ac52_sc7852_io_space`, `lib10_ac52_written_views_of_me1_and_io`, `lib10_ac52_every_bank_of_the_pc1600_and_its_modules` (a CE-1600M in Slot 1), `lib10_ac52_vertical_banks_of_a_slot_2_module` (a CE-1601M in Slot 2); `harness_tests.cpp`: `lib10_ac52_harness_peek_bank_and_vbank` |

D-16 (`step` with a test): `library/debugger/tests/run_control_tests.cpp`
`step_until_goes_on_while_the_test_says_so`, `step_until_ends_at_a_breakpoint`
(`calcu_library_tests`).

D16-12 (the DAP on the new layer): `CalcU1600DapSmoke` (Longer) passes
against the Qt app, every scenario, twice in a row (26 s). The old finding
(no `stopped` after the first `stepIn`) did not show; a fault of the new
code that looked like it did (a stop that landed within the next host
call was not told) is fixed (`DebugController::noteRunning`).

### Golden and reference runs

- **Reference traces** (§10.11), recorded before the core was touched
  (commit `3bbf639`) with today's path: `tests/golden-runs/pc1600-basic.trace`
  (a PC-1600 cold booted and run 29 frames: 204'900 instructions, 6.5 MB)
  and `pc1500a-basic.trace` (a PC-1500A, 120 frames: 70'252
  instructions, 2.0 MB). `lib10_ac42_bytes_equal_the_recorded_reference`
  writes the same runs through `SharpTraceRecorder` (into a byte buffer since
  P-5, design §11, as `TraceSession` takes them): byte for byte equal, but for the eight bytes of the session
  start's time (bytes 19 to 26), which the format holds and no run can
  repeat. The design named the first 200'000 and 100'000 instructions;
  the runs stop at a frame instead (the PC-1500A idles, 120 frames are
  70'252 instructions).
- **Golden runs**: all 23 replay unchanged.
- **Speed (R-1)**: `CalcU1600DebuggerSpeed` (Longer) runs a PC-1600 for 20
  s of emulated time at full speed after its cold boot: 0.22 s without and
  0.20 s with a breakpoint that is never reached (about 90 times real
  time either way). 0.8.1 was not measured: its pin was not built for
  this step (finding).

### Deviations

- **Bank windows (AC-49, AC-52).** The SC7852's `memory` is banked per
  page: A and D by two banks, B and C by eight, page C also by 256
  vertical banks (port 28H's byte; the CE-1601M fits 2, the superRAM 16).
  A peek or poke with `bank=` sets port 31H's field of the page for the
  access; with `vbank=` it drives OUT (28H) on the bus (the module's
  latch) and back. The LH5801 and LH5803 bank `me0` and `me1` by PU and
  PV (`bank` = PU × 2 + PV); there are no vertical banks on their side.
- **The `io` space** reads only the bank registers (28H, 31H, 3CH) and
  is unreadable at every other port; its `-written` view holds the CPU's
  last OUT of each port. The `-written` views hold the last value written
  to every cell (not only those that read back unreadable).
- **Watches** on `io` and on the `-written` views are taken and never hit
  (the CPUs watch memory and ME1 only).
- **Registers' names** are those of `CpuRegisters.cpp` (`af'`, `bc'`, …
  on the SC7852); every register is writable.
- **The observer** sees the machine after each machine step; a halted
  CPU's idle ticks are no instruction. During a power cycle's phase the
  checks apply too (no exception for the power-cycle sequence).
- **The trace limit.** At the limit the records waiting in the core's
  buffer are dropped and only SESSION_END is written (the file stays
  within the limit); its event count is the core's, which may count
  frames that did not fit.
- **Today's tools keep their file**: `beginCpuTrace(FILE*)` stays for
  the CLIs' preset loader (without `traceHook`); `cpuTraceActive()` is
  true for a file only, so the preset runner's `TraceCloser` leaves the
  app's records alone. Part D (P-4, `ds-core`): the file mode and the four
  `…CpuTrace…` functions deleted (the CLIs went in M-51);
  `lib10_ac44_trace_flags_leave_breakpoints_alone` uses the records.
- **The DAP's events**: `DebugController` compares the `run-state` topic
  after each host call; a resume, step or pause the session makes counts
  as running first, so a stop that lands within the next call, or a pause
  of a machine that was paused already (a reset's entry stop), is told
  once, as 0.8.1 told it.
- **The DAP's target view** casts the library's targets to
  `sharp::SharpDebugTarget` for the registers by name, the bank state and
  the history's bytes (design §3.6 says "over `machine::DebugTarget`").
- **`SessionFixture`-free rig.** `tests/SharpDebugRig.hpp` builds an app
  session with only the `DebuggerPart` around a Sharp machine the test
  builds (a ROM-less PC-1500A for the memtest fixture, as 0.8.1's tests).
- **GUI check verb `command`** sends a session command from a scenario
  (until the scenarios become an attach client in M-49).

### Findings

- The speed against 0.8.1 is not measured (above).
- CI should be dispatched at the merge (LIB-8's macOS clipboard; the core
  and the Qt6 debugger changed throughout).

### Calc-U-1600: the format 2 clauses (M-46, 2026-11-16, branch `m46-presets-1600`)

| ID | Result | Test case |
|---|---|---|
| AC-40 (the preset clause) | Pass | `lib10_ac40_debugger_steps_refused_at_read` (`sharp_presets_tests.cpp`: a debugger step in a format 2 preset without a debugger refused when read, with its line); Web `session.spec.js` `lib10_ac40 trace step fails on the web` (EC-8) |
| AC-43 | Pass on format 2 | `lib10_ac43_preset_trace_ends_at_the_limit_with_a_notice` (the `trace:` step of a format 2 preset), `lib10_ac43_button_trace_ends_at_the_limit_with_a_notice` |

The DAP's clean start loads the attach configuration's preset or the
model's startup preset with `load-preset {buildOnly}`, else `cold-boot`;
its `debug:` block comes from `SharpDebugBlock::debugBlockOf` (C1600-4
AC-12). `CalcU1600DapSmoke` passed (26 s). Every criterion of the column
passes: **Done**.

## Calc-U-59 (M-53, 2026-11-16, branch `m53-paper-debugger-59`)

Built and run as LIB-8's Calc-U-59 column (same step, same trees).

The code (design §12.3): `core/TIDebugging.*` (new: `TIDebugging`, the
target `TIDebugTarget` TMC0501, `TITraceRecords`; the marks of
`InstructionEvent`: boundary, scan loop, weight); `core/TraceRecords.hpp`
(new: the product kind `c59.trace-records`); `TIMachine::advance` asks the
library's observer while Checks are set and stops before a breakpoint;
`TIDebugTargetAdapter`, `TIStepObserver` and `TIStop` go, and the panel's
reads move to `TIDebugPanelAdapter` (L-E3); `app/TILevelRules.*`,
`app/TITraceRecorder.*`, `app/TIDebuggerSetup.*` (new);
`app/TraceWriter.*` is an encoder of records only; `app/FreezeController.*`
deleted; `EmulatorSession` keeps the playback, the places, the frame
sampler and the queued cards (the halt flag, the queued steps, the freeze
state, the two trace writers, `advanceHosted` and `TIKeyTraceObserver`
go); `TIAppPart` (the `c59.freeze` … `c59.step-instruction`,
`c59.manual-trace` commands and the `c59.trace` topic go; `c59.run-state`
keeps `fullSpeed` and `keystrokesPlaying`, L-E5; the KEYSTROKES directives
through the run control, L-E1; `tellRestarted` after Reset and Clean,
L-E2; the frozen view rebuilt when the run state changes; `debug-panel:`
views through `showDebugPanelNamed`, L-E4); `makeAppSession` owns a
`DebuggerPart` after the `PaperPart` (LIB-8 D-23); `HarnessProduct`
(`debugger`); `TIWebPart` (`c59.web.traces`, `c59.web.trace-ready`,
`c59.web.trace-bytes` and the MEMFS `/traces` go; `c59.web.run-state`
from the run state); the headless tool steps with `Machine::advance(1)`
and reads frames through `TIDebugPanelAdapter`.
Frontends: Qt6 `debug/LiveDebugWidget` (level `program`) and
`debug/CpuInspectorWidget` (level `cpu`) drive FREEZE, STEP and RESUME with
a `calcu::qt6::RunControlButtons` each, FREEZE ON START and ARMED send
`pause-on-start`, `LogWidget`'s TRACE sends `trace {button}` and shows the
`trace` topic, `HoldRepeatButton` deleted; Web `Debugger.jsx` with
`CalcuRunControlButtons` per tab, TRACE on the library topic, the list of
ended traces kept by the page from the `keep-file` hand-overs (D-25),
`engine.js`'s freeze, step and trace methods gone; Swift: the bridge
mirrors `run-state` and `trace`, sends `pause`, `step`, `resume`,
`pause-on-start`, `trace`, and wraps `calcu::debugger::buttonUse`
(D-27); `HoldRepeatButton` repeats every 250 ms while step can be used.

| ID | Result | Test case |
|---|---|---|
| AC-28 | Pass | `debug_target_tests.cpp`: `lib10_ac28_one_target_tmc0501_on_every_model`, `_registers_of_the_processor_frame`, `_spaces_rom_ram_scom_program`, `_ram_and_program_written_rom_scom_refused`, `_history_and_decoding_no_watches_no_over_out`, `_conformance_on_every_model` (`checkDebugging`); `descriptor_tests.cpp` (the `c59.trace-records` capability) |
| AC-29 | Pass | `debugger_tests.cpp`: `lib10_ac29_pause_lands_at_the_next_boundary`, `_waiting_for_a_key_lands_after_50000` (50000 to 50003 weights), `_in_an_overlay_lands_after_50000` (the rule without boundaries), `_program_step_is_one_program_byte_as_sst`, `_program_step_through_rom_and_module_programs` (the Master Library's program 01), `_cpu_step_repeats_a_hold_up_to_32` |
| AC-30 | Pass, with row 59-34 in place of its Reset clause (Q-1, user 2026-11-16) | `debugger_tests.cpp`: `lib10_ac30_program_fires_at_the_next_boundary`, `_cpu_fires_leaving_key_readiness` (was `freeze_on_scan_loop_exit_fires_on_key`), `_cpu_fires_at_0000_after_reset_and_cold_boot` (Reset and Clean on the TI-59, TI-58 and TI-58C; the 0000 rule), `_arming_one_disarms_the_other` (was `freeze_on_pc_change_and_mutual_exclusion`), `_ti58_card_veto_is_part_of_the_scan_loop`; `part_flows_tests.cpp` `lib10_ac30_part_cpu_freeze_on_start_sees_reset_and_clean_reset` |
| AC-31 | Pass | `harness_tests.cpp` `lib10_ac31_harness_breakpoint_pauses_the_ti` (`targets`, `registers`, `break`, `history`); `debugger_tests.cpp` `lib10_ac31_app_session_pauses_at_a_breakpoint` (setter `harness`; before the instruction; passed once at resume) |
| AC-32 | Pass (2.3.0 directives through L-E1; format 2 in M-54) | `debugger_tests.cpp`: `lib10_ac32_freeze_calculator_is_pause_program`, `_freeze_cpu_is_pause_cpu`, `_freeze_on_start_is_pause_on_start`, `_step_and_resume` (and `Step:` while running: the log line, OQ-4), `_debug_panel_calculator_cpu_log`; `part_flows_tests.cpp` `part_keystroke_controls_drive_the_gui`; the reference run `screentest_test_cdebug.ti59` unchanged |
| AC-33 | Pass | `trace_tests.cpp`: `lib10_ac33_header_ti59_version_2_and_model` (writes `build/trace-test.bin`, which CTest `CalcU59TraceReader` reads with `tools/read_trace.py`), `_records_session_instruction_gap` (and whole records within a budget), `_no_key_or_card_records` (the button's trace with a key and a card), `_button_names_by_model`, `_limit_ends_with_a_notice`; `golden_runs_tests.cpp` `lib10_ac33_bytes_equal_the_recorded_reference`; the exit check `lib10_ac33_exit_check_preset_and_button_traces_are_one_session` |
| AC-34 | Pass in Qt6 and the Web; Swift not run | Qt6 `CalcU59QtTests` `lib10_ac34_qt6_calculator_and_cpu_buttons_follow_run_state`; Web `play-calculator.spec.js` `LIB-10 AC-34 FREEZE STEP RESUME of both tabs follow the run state`; Swift `Calc-U-59Tests/RunControlTests.swift` `testLib10AC34ButtonsFollowTheRule` (new; type-checked only: the Xcode build fails in the worktree) |
| AC-35 | Pass | ported onto the run control: `debugger_tests.cpp` `lib10_ac35_session_queued_steps_run_after_the_host_call`, `_session_freeze_fires_through_listener`, `_session_steps_only_when_frozen_and_arms_exclude`, `_c59_1_runbatch_stops_at_freeze_inside_advance`; `shared_debug_tests.cpp` `lib10_ac35_step_shows_the_next_step`, `_running_view_shows_only_step_boundaries`; `part_flows_tests.cpp` `lib10_ac35_part_model_switch_keeps_printer_and_log_resets_freeze`, `_part_resume_before_the_frozen_view_drops_it`, and `part_load_while_frozen_ends_running_without_tracing`, `part_constant_memory_keeps_the_loaded_card`, `part_asm_overlay`, `part_events_of_a_replaced_machine_are_ignored` on it; `typing_tests.cpp` `lib5_ac20_freeze_inside_hold_key_up_on_frozen_machine`. `trace_flags_for_consumers` went with `TraceConsumers` |
| AC-26 (59: the Web's hand-over) | Pass | Web `play-calculator.spec.js` `LIB-10 AC-26 a trace is downloaded when switched off` (renamed; `CALCU59_TRACE.bin`) |

Exit check of M-53 (a trace of a preset and the button's are one
session): `lib10_ac33_exit_check_preset_and_button_traces_are_one_session`
starts the button's trace, then a preset's `Trace:` ends it and runs alone
until the preset ends.

### Golden and reference runs

- Before the core was touched, the trace of `tests/golden-runs/
  ti59-calculation.run` was recorded on 2.3.0's path (TraceWriter, the ring
  drained every 256 instructions): 5,570,343 bytes, no key or card record
  (the script presses keys on the machine). It is kept as its length and
  MD5 (`921b0aeb…`) with the SESSION_START seconds zeroed, not as the file
  (5.5 MB). The trace recorder's file of the same run equals it byte for
  byte (`lib10_ac33_bytes_equal_the_recorded_reference`).
- The 18 golden runs replay unchanged (no observer: the fast path).
- The 57 reference runs (with `debug_*` and `screentest_test_cdebug`,
  which freeze and step through L-E1) match their `.txt` files with
  LIB-5's accepted differences unchanged.

### Deviations

- **A pause or a step that waits to land runs on in the host call that
  asked for it** (`TIDebugging::setRunOn`, set by `TIAppPart` while the
  run control is neither running nor paused; at most 50000 × 4 weights),
  as 2.3.0's freeze and STEP ran there. Without it a FREEZE of the idle
  calculator would land after about 3.4 s of paced time (50000 weights).
  The harness has no run-on: there a pause lands in emulated time.
- **Breakpoints are checked by `TIMachine::advance`** before each
  instruction, not by the core's `TRACE_BREAKPOINTS` (which stops after
  the instruction at the address); the core's breakpoints stay for the
  headless tools (`breakpoint_sets_step_bit31`).
- **The frame ring** is on while the debug panel wants it (the CPU tab
  shows, paused, a start armed), while a trace runs, and while the run
  control's checks are set (the target's history, AC-28); a trace starts
  with the ring emptied.
- **Pause where the machine stands.** The ASM overlay's entry with CPU
  FREEZE ON START armed pauses in place through
  `TILevelRules::landNextPauseAtOnce()` (2.3.0's `freezeInPlace`, until
  C59-4 in M-56).
- **A restart keeps what the `cpu` start has seen** (`TILevelRules::
  startArmed` at 0000 for the level armed), so a Reset fires it as 2.3.0's
  `FreezeController` did (row 59-34).
- **A trace that cannot start fails its 2.3.0 preset** (L-E1): the notice
  "Trace error: KEYSTROKES "Trace: …": <reason>" and the KEYSTROKES stop.
- **The reference trace** is a digest (above), not `ti59-calculation.trace`.
- **The read_trace check** stays the CTest `CalcU59TraceReader` on
  `build/trace-test.bin`, now written by the trace recorder, instead of a
  new `CalcU59ReadTrace` through the harness.
- **`typing::PauseProbe`** stays in the library: the harness's test product
  still uses it (§12.7 removes it only when no user is left).
- **Qt6** keeps FREEZE, STEP and RESUME in the tab's header row (the
  library widget drives them) in today's order and colours; **Web**: their
  order is FREEZE STEP RESUME, then FREEZE ON START, ARMED (today RESUME,
  FREEZE, FREEZE ON START, ARMED, STEP). In both FREEZE can be used while a
  start is armed (the library's button rule; today it was disabled).
- **Swift's test** checks a copy of the seven rows of
  `library/debugger/tests/button_cases.json` kept in the test (the sandboxed
  test host may not read the source tree), and the library's file when it
  can be read.

### Findings

- The trace limit is `[trace] max-file-size-mb` MiB (LIB-10 T2); 2.3.0
  counted 1,000,000 bytes per MB. Added to ledger row 59-48.
- `screentest_test_cdebug.ti59` and `screentest_test_58_glyphs.ti58` still
  end with their operation running (the accepted "playback:" difference of
  M-34): the preset's last key after `FreezeOnStart:` is held by the typing
  engine until the ROM is ready for keys, which the paused machine never is.
  M-34 expected M-53 to end it; it does not (the reason in
  `reference_runs_tests.cpp` says so now).
- The Swift app was not built and `UIRegressionTests` not run (above); the
  XCUITests that press FREEZE, STEP and RESUME
  (`ScreenshotTests.swift:150-160, 229-271`) now wait for a step to land.

AC-34's Swift part is not verified in this step, so the column stays **In
progress**; it is Done once the Swift build and its tests pass (the user's
check of M-53).
